///////////////////////////////////////////////////////////////////////////////
// PWM
///////////////////////////////////////////////////////////////////////////////

struct pwmOutput_struct {
  unsigned char pwmPin;
  unsigned char dirPin;
  int  pwmValTarget;
  int  pwmValCurrent;
  int  pwmAcc;
} pwmOutput[] = {
//      {  3,  4, 0, 0 },
        {  9,  8, 0, 0 },
//      { 10, 12, 0, 0 },
};
#define pwmOutput_n       ((sizeof(pwmOutput)) / (sizeof(struct pwmOutput_struct)))
#define pwmOutputMin      60
#define pwmOutputMax      (255 - pwmOutputMin)
#define pwmOutputAccel    7
#define pwmOutputMaxAccel 40

int pwmOutputSelected = 0;

void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

void pwmSetup(void)
{
  for(int i = 0; i < pwmOutput_n; i++) {
    // Set pins PWM frequency to 31250 Hz (31250/1 = 31250)
    //
    // Following base freqs and divisors are valid
    //   - Base frequencies:
    //      o The base frequency for pins 3, 9, 10, and 11 is 31250 Hz.
    //      o The base frequency for pins 5 and 6 is 62500 Hz.
    //   - Divisors:
    //      o The divisors available on pins 5, 6, 9 and 10 are: 1, 8, 64,
    //        256, and 1024.
    //      o The divisors available on pins 3 and 11 are: 1, 8, 32, 64,
    //        128, 256, and 1024.
    //
    // PWM frequencies are tied together in pairs of pins. If one in a
    // pair is changed, the other is also changed to match:
    //   - Pins 5 and 6 are paired on timer0
    //   - Pins 9 and 10 are paired on timer1
    //   - Pins 3 and 11 are paired on timer2
    //
    // Note that this function will have side effects on anything else
    // that uses timers:
    //   - Changes on pins 3, 5, 6, or 11 may cause the delay() and
    //     millis() functions to stop working. Other timing-related
    //     functions may also be affected.
    //   - Changes on pins 9 or 10 will cause the Servo library to function
    //     incorrectly.
    setPwmFrequency(pwmOutput[i].pwmPin, 1);
    analogWrite(pwmOutput[i].pwmPin, 0);

    // Direction PIN
    pinMode(pwmOutput[i].dirPin, OUTPUT);
    digitalWrite(pwmOutput[i].dirPin, LOW);
  }
  
  Serial.print("sizeof(int) = "); Serial.println(sizeof(int));
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

void pwmRefresh(void)
{
  int d, target_speed, acc_inc;
  
  for(int sp = 0; sp < pwmOutput_n; sp++) {
    if(pwmOutput[sp].pwmValTarget != pwmOutput[sp].pwmValCurrent) {
//Serial.print(pwmOutput[sp].pwmValTarget & (int) 0x8000); Serial.print(" - "); Serial.println(pwmOutput[sp].pwmValCurrent & (int) 0x8000);
      // If target of different sign than current val then brake to zero...
      // elsewhere go directly to the target speed
      target_speed = pwmOutput[sp].pwmValCurrent && ((pwmOutput[sp].pwmValTarget & (int) 0x8000) ^ (pwmOutput[sp].pwmValCurrent & (int) 0x8000))
                       ? 0
                       : pwmOutput[sp].pwmValTarget;
      
      // calculate acceleration increment
      acc_inc = target_speed > pwmOutput[sp].pwmValCurrent ? pwmOutputAccel : -pwmOutputAccel;
      if(abs(pwmOutput[sp].pwmAcc) < pwmOutputMaxAccel)
        pwmOutput[sp].pwmAcc += acc_inc;
        
      // calculate
      if(abs(target_speed - pwmOutput[sp].pwmValCurrent) < abs(pwmOutput[sp].pwmAcc)) {
        d = (target_speed - pwmOutput[sp].pwmValCurrent) >> 1;
        if(!d) d = target_speed - pwmOutput[sp].pwmValCurrent;
        pwmOutput[sp].pwmAcc = d;
      } else {
        d = pwmOutput[sp].pwmAcc;
      }

      pwmOutput[sp].pwmValCurrent += d;
      
      if(pwmOutput[sp].pwmValTarget == pwmOutput[sp].pwmValCurrent)
        pwmOutput[sp].pwmAcc = 0;

      analogWrite(pwmOutput[sp].pwmPin, pwmOutputMin + abs(pwmOutput[sp].pwmValCurrent));
      digitalWrite(pwmOutput[sp].dirPin, pwmOutput[sp].pwmValCurrent < 0 ? LOW : HIGH);
      
      pwmPrintStatus(sp);
    }
  }
    /*
    d = pwmOutput[sp].pwmValTarget - pwmOutput[sp].pwmValCurrent;
    if(d) {
      if(d < 0 && d < -5) d = -5;
      if(d > 0 && d > 5)  d = 5;
    */
}

void pwmSelect(int d)
{
  if(d < 0) {
    if(pwmOutputSelected > 0)
      pwmOutputSelected--;
  } else {
    if(pwmOutputSelected + 1 < pwmOutput_n)
      pwmOutputSelected++;
  }

  Serial.print("Selected PWM output [");
  Serial.print(pwmOutputSelected);  
  Serial.println("]");
}

#define pwmSelectPrevious() pwmSelect(-1)
#define pwmSelectNext()     pwmSelect(1)

void pwmAccelerate(int v)
{
  pwmOutput[pwmOutputSelected].pwmValTarget =
          v > 0
              ? (pwmOutput[pwmOutputSelected].pwmValTarget + v < pwmOutputMax
                  ? pwmOutput[pwmOutputSelected].pwmValTarget + v
                  : pwmOutputMax)
              : (pwmOutput[pwmOutputSelected].pwmValTarget + v > -pwmOutputMax
                  ? pwmOutput[pwmOutputSelected].pwmValTarget + v
                  : -pwmOutputMax);
}

void pwmStop(int sp)
{
  if(sp < 0 || sp > pwmOutput_n) {
    for(sp = 0; sp < pwmOutput_n; sp++) {
      analogWrite(pwmOutput[sp].pwmPin, 0);
      digitalWrite(pwmOutput[sp].dirPin, LOW);
      pwmOutput[sp].pwmValCurrent = pwmOutput[sp].pwmValTarget = 0;
    }
    Serial.println("EMERGENCY STOP!");
  } else {
    pwmOutput[sp].pwmValTarget = 0;
  }
}

#define pwmStopEmergency() pwmStop(-1)
#define pwmStopCurrent()   pwmStop(pwmOutputSelected)

void pwmSwitchDirection()
{
  pwmOutput[pwmOutputSelected].pwmValTarget = 0 - pwmOutput[pwmOutputSelected].pwmValTarget;
}

///////////////////////////////////////////////////////////////////////////////
// JOYSTCICK
///////////////////////////////////////////////////////////////////////////////

const int joyAxisXPin  = A0;
const int joyAxisYPin  = A1;
const int joyButtonPin = A2;
const int joyAxisXRange = 1023;
const int joyAxisYRange = 1023;
const int joyButton = A2;

int joyStatusNow = 0;
int joyStatusOld = 0;

#define JOY_UP     0x01
#define JOY_DOWN   0x02
#define JOY_LEFT   0x04
#define JOY_RIGHT  0x08
#define JOY_BUTTON 0x10

#define joyMove(x)         (joyStatusNow & (x))
#define joyPressed(x)      (!(joyStatusOld & (x)) && (joyStatusNow & (x)))

#define joyMoveUp()        joyMove(JOY_UP)
#define joyMoveDown()      joyMove(JOY_DOWN)
#define joyMoveLeft()      joyMove(JOY_LEFT)
#define joyMoveRight()     joyMove(JOY_RIGHT)

#define joyPressedLeft()   joyPressed(JOY_LEFT)
#define joyPressedRight()  joyPressed(JOY_RIGHT)
#define joyPressedButton() joyPressed(JOY_BUTTON)

void joySetup(void)
{
}

void joyPrint()
{
  Serial.print("joyRead() =");
  Serial.print(analogRead(joyButtonPin));
  if(joyMoveUp())         Serial.print(" UP");
  if(joyMoveDown())       Serial.print(" DOWN");
  if(joyMoveLeft())       Serial.print(" LEFT");
  if(joyMoveRight())      Serial.print(" RIGHT");
  if(joyMove(JOY_BUTTON)) Serial.println(" [XX]"); else Serial.println(" [  ]");
}

int joyRead()
{
  int v;
  
  // reset joystick status
  joyStatusOld = joyStatusNow;
  joyStatusNow = 0;

  // update X axis
  v = analogRead(joyAxisXPin);
  if     (v < (joyAxisXRange >> 2))                        joyStatusNow |= JOY_LEFT;
  else if(v > (joyAxisXRange >> 1) + (joyAxisXRange >> 2)) joyStatusNow |= JOY_RIGHT;
  
  // update Y axis
  v = analogRead(joyAxisYPin);
  if     (v < (joyAxisYRange >> 2))                        joyStatusNow |= JOY_UP;
  else if(v > (joyAxisYRange >> 1) + (joyAxisYRange >> 2)) joyStatusNow |= JOY_DOWN;
  
  // update button
  if(analogRead(joyButtonPin) == 0) joyStatusNow |= JOY_BUTTON;
  
  // debug
  //joyPrint();
}

void setup()
{
  int i;
  Serial.begin(9600);
  Serial.println("Initializing");
  
  joySetup();
  pwmSetup();
}


void loop()
{
  joyRead();
  
  if(joyPressedButton()) {
    //pwmSwitchDirection();
    pwmStopCurrent();
    //pwmStopEmergency();
  } else
  if(joyMoveUp()) {
    pwmAccelerate(10);
  } else
  if(joyMoveDown()) {
    pwmAccelerate(-10);
  } else
  if(joyPressedLeft()) {
    pwmSelectPrevious();
  } else
  if(joyPressedRight()) {
    pwmSelectNext();
  }
  
  pwmRefresh();
  
  delay(100);
}

