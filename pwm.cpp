///////////////////////////////////////////////////////////////////////////////
// PWM
///////////////////////////////////////////////////////////////////////////////

struct pwmOutput_struct {
  int           pwmValTarget;
  int           pwmValCurrent;
  int           pwmAcc;
  unsigned char pwmMode;
} pwmOutput[BOOSTER_N];

#define PWM_OUTPUT_N              BOOSTER_N
#define PWM_OUTPUT_MIN            60
#define PWM_OUTPUT_MAX            (255 - PWM_OUTPUT_MIN)
#define PWM_OUTPUT_ACCEL          7
#define PWM_OUTPUT_MAX_ACCEL      40

struct booster_mngr_struct pwmBoosterMngr =
  { "PWM", pwmStart, pwmStop, pwmRefresh };

void pwmStart(void)
{
  disable_interrupts();

  // CONFIGURE TIMERS
  // timer 1
  TCCR1A = (TCCR1A & 0b00001100)
         | 0b00000001;  // Fast PWM 8-bit (!WGM11, WGM10)
  TCCR1B = (TCCR1B & 0b11100000)
         | 0x4          // clock prescaler 0x04 (16e6/256 = 62.5 khz)
         | 0b00001000;  // Fast PWM 8-bit (!WGM13, WGM12)
  OCR1A = OCR1B = 0;    // set compare registers to 0
  TIMSK1 &= 0b11011000; // disable all timer 1 interrupts

  // timer 2
  TCCR2A = (TCCR2A & 0b00001100)
         | 0b00000011;  // Fast PWM (WGM21, WGM20)
  TCCR2B = (TCCR2B & 0b11110000)
         | 0x6          // clock prescaler 0x6 (16e6/256 = 62.5 khz)
         | 0b00000000;  // Fast PWM (!WGM22)
  OCR2A = OCR2B = 0;    // set compare registers to 0
  TIMSK2 &= 0b11111000; // disable all timer 2 interrupts
  
  // CONFIGURE OUTPUT PINS FOR DIRECT PWM OUTPUT
  for(int b = 0; b < BOOSTER_N; b++) {
    switch(booster[b].pwmSignalPin) {
      case 9:  TCCR1A |= 0b10000000; break; // Clear OC1A on cmp match
      case 10: TCCR1A |= 0b00100000; break; // Clear OC1B on cmp match
      case 11: TCCR2A |= 0b10000000; break; // Clear OC2A on cmp match
      case 3:  TCCR2A |= 0b00100000; break; // Clear OC2B on cmp match
      default: fatal("Cannot configure output pin for PWM output");
    }
  }

  //-------------------------------------------
  // RESET PWM STATUS
  for(int i = 0; i < BOOSTER_N; i++) {
    memset(pwmOutput + i, 0, sizeof(struct pwmOutput_struct));
    pwmOutput[i].pwmMode = PWM_OUTPUT_MODE_INERTIAL;
  }

  enable_interrupts();
}

void pwmStop(void)
{
  disable_interrupts();

  // disconnect all OC1A:B/OC2A:B pins and go to normal operations mode
  TCCR1A = TCCR1A & 0b00001100;    TCCR2A = TCCR2A & 0b00001100;
  TCCR1B = TCCR1B & 0b11100000;    TCCR2B = TCCR2B & 0b11110000;
  OCR1A = OCR1B = 0;               OCR2A = OCR2B = 0;
  TIMSK1 &= 0b11011000;            TIMSK2 &= 0b11111000;

  enable_interrupts();
}

void pwmRefreshPwmOutput(int sp)
{
  switch(pwmOutput[sp].pwmMode) {
    case PWM_OUTPUT_MODE_INERTIAL:
      if(pwmOutput[sp].pwmValTarget != pwmOutput[sp].pwmValCurrent) {
        int d, target_speed, acc_inc;
        
        // If target of different sign than current val then brake to zero...
        // elsewhere go directly to the target speed
        target_speed = pwmOutput[sp].pwmValCurrent && (pwmOutput[sp].pwmValTarget ^ pwmOutput[sp].pwmValCurrent) & ((int) 0x8000)
                         ? 0
                         : pwmOutput[sp].pwmValTarget;
      
        // calculate acceleration increment
        acc_inc = target_speed > pwmOutput[sp].pwmValCurrent ? PWM_OUTPUT_ACCEL : -PWM_OUTPUT_ACCEL;
        if(abs(pwmOutput[sp].pwmAcc) < PWM_OUTPUT_MAX_ACCEL)
          pwmOutput[sp].pwmAcc += acc_inc;
        
        // calculate final speed
        if(abs(target_speed - pwmOutput[sp].pwmValCurrent) < abs(pwmOutput[sp].pwmAcc)) {
          d = (target_speed - pwmOutput[sp].pwmValCurrent) >> 1;
          if(!d)
            d = target_speed - pwmOutput[sp].pwmValCurrent;
          pwmOutput[sp].pwmAcc = d;
        } else {
          d = pwmOutput[sp].pwmAcc;
        }
        pwmOutput[sp].pwmValCurrent += d;
      
        if(pwmOutput[sp].pwmValTarget == pwmOutput[sp].pwmValCurrent)
          pwmOutput[sp].pwmAcc = 0;
      }
      break;
      
    case PWM_OUTPUT_MODE_DIRECT:
      pwmOutput[sp].pwmValCurrent = pwmOutput[sp].pwmValTarget;
      pwmOutput[sp].pwmAcc = 0;
      break;
        
    default:
      fatal("Unknwon pwm mode");
  }

  //pwmPrintStatus(sp);

  digitalWrite(booster[sp].dirSignalPin, pwmOutput[sp].pwmValCurrent > 0);
  unsigned char pwmvalue = PWM_OUTPUT_MIN + abs(pwmOutput[sp].pwmValCurrent);
  switch(booster[sp].pwmSignalPin) {
    case  3: OCR2B = pwmvalue; break;
    case  9: OCR1A = pwmvalue; break;
    case 10: OCR1B = pwmvalue; break;
    case 11: OCR2A = pwmvalue; break;
    default: fatal("Cannot write PWM output to pin");
  }
}

void pwmRefresh(void)
{
  for(int sp = 0; sp < PWM_OUTPUT_N; sp++)
    pwmRefreshPwmOutput(sp);
}

void pwmPrintStatus(int sp)
{
  Serial.print("PWM[");
  Serial.print(sp);
  Serial.print("] = [");
  Serial.print(pwmOutput[sp].pwmValCurrent < 0 ? '-' : '+');
  Serial.print(pwmOutput[sp].pwmValCurrent);
  Serial.print("] -> [");
  Serial.print(pwmOutput[sp].pwmValTarget < 0 ? '-' : '+');
  Serial.print(pwmOutput[sp].pwmValTarget);
  Serial.print("] (acc=");
  Serial.print(pwmOutput[sp].pwmAcc);
  Serial.println(")");
}


void pwmAccelerate(int ps, int v)
{
  if(ps < 0 || ps > BOOSTER_N)
    fatal("pwmAccelerate: pwm out of bounds.");
    
  pwmOutput[ps].pwmValTarget =
          v > 0
              ? (pwmOutput[ps].pwmValTarget + v < PWM_OUTPUT_MAX
                  ? pwmOutput[ps].pwmValTarget + v
                  : PWM_OUTPUT_MAX)
              : (pwmOutput[ps].pwmValTarget + v > -PWM_OUTPUT_MAX
                  ? pwmOutput[ps].pwmValTarget + v
                  : -PWM_OUTPUT_MAX);
}

void pwmSpeed(int ps, int s)
{
	if(ps < 0 || ps > BOOSTER_N)
		fatal("pwmSpeed: pwm booster out of bounds.");
	if(s < -255 || s > 255)
		fatal("pwmSpeed: speed out of bounds.");

	pwmOutput[ps].pwmValTarget = s;
}

void pwmStop(int sp)
{
  if(sp < 0 || sp > PWM_OUTPUT_N)
    fatal("pwmStop: pwm out of bounds");

  pwmOutput[sp].pwmValTarget = 0;
}

void pwmSwitchDirection(int ps)
{
  if(ps < 0 || ps > BOOSTER_N)
    fatal("pwmSwitchDirection: pwm out of bounds.");
    
  pwmOutput[ps].pwmValTarget = 0 - pwmOutput[ps].pwmValTarget;
}

