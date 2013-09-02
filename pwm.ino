#include <avr/interrupt.h>

///////////////////////////////////////////////////////////////////////////////
// ERROR HANDLING
///////////////////////////////////////////////////////////////////////////////

void fatal(char *msg)
{
  boosterEmergencyStop(-1);
  Serial.print("FATAL ERROR");
  if(msg) {
    Serial.print(": ");
    Serial.println(msg);
  }
  while(1);
}

///////////////////////////////////////////////////////////////////////////////
// HW MANAGEMENT
///////////////////////////////////////////////////////////////////////////////

unsigned char sreg;
void disable_interrupts(void)
{
  sreg = SREG;
  cli();
}

void enable_interrupts(void)
{
  sei();
  SREG = sreg;
}

unsigned int (*timerHandler)(void) = NULL;

void timerSetHandler(unsigned int (*th)(void))
{
  timerHandler = th;
}

unsigned int timerGetMaxCount(int timer)
{
  return timer == 0 || timer == 2 ? 255 : 65535;
}

// For PWM use:
//   - Pins 5 and 6 are paired on timer0
//   - Pins 9 and 10 are paired on timer1
//   - Pins 3 and 11 are paired on timer2
void timerSetModePrescaler(int timer, int mode, int prescaler, unsigned int first_cycle)
{
  switch(timer) {
    case 0:
      fatal("timer#0 is not configurable");
      break;
      
    case 1:
      TCCR1A = 0;
      TCCR1B = (TCCR1B & 0b11111000) | prescaler;
      TCNT1 = first_cycle;
      TIMSK1 = 1 << TOIE1;
      break;

    case 2:
      TCCR2A = 0;
      TCCR2B = (TCCR2B & 0b11111000) | prescaler;
      TCNT2 = first_cycle;
      TIMSK2 = 1 << TOIE2;
      break;

    default:
      fatal("Bad timer selected");
  }
}

void timerSetSlowest(int timer, int mode)
{
  switch(timer) {
    case 0:
      fatal("timer#0 is not configurable");
      break;
      
    case 1:
      timerSetModePrescaler(timer, mode, 5, 0);
      break;

    case 2:
      timerSetModePrescaler(timer, mode, 7, 0);
      break;

    default:
      fatal("Bad timer selected");
  }
}

void timerSetInterruptFrequency(int timer,
                               float timeoutFrequency,
                               unsigned int *counter_base,
                               unsigned int *counter_max)
{
  float freqs01[] = { 1, 8, 64, 256, 1024, 0.0 };
  float freqs2[]  = { 1, 8, 32, 64, 128, 256, 1024, 0.0 };
  float *freqs, counter, counter_size;
  
  // some input params checks
  if(timer < 0 || timer > 2)
    fatal("Invalid timer");
  if(!counter_base)
    fatal("NULL counter_base");
  if(!counter_max)
    fatal("NULL counter_max");
  
  // select prescaled frequency table and get counter_max
  freqs = timer == 2 ? freqs2 : freqs01;
  for(int i; freqs[i] != 0; i++)
    freqs[i] = ((float) F_CPU) / freqs[i];
  *counter_max = timerGetMaxCount(timer);
  
  Serial.print("timeoutFrequency ");
    Serial.print(timeoutFrequency);
    Serial.print(" on timer ");
    Serial.println(timer);
    
  for(int prescaler = 0; freqs[prescaler] != 0.0; prescaler++) {
    counter = freqs[prescaler] / timeoutFrequency;
    Serial.print("counter = "); Serial.println(counter);
    if(counter < 1.0)
      fatal("ERROR: Cannot give so high res :(");
    if(counter <= (float) *counter_max) {
      *counter_base = (unsigned int) ((((float) *counter_max) - counter) + 0.5); //the 0.5 is for rounding;
      
      Serial.print("timerSetInterruptFrequency: Using prescaler");
        Serial.print(prescaler + 1);
        Serial.print(" (f=");
        Serial.print(freqs[prescaler]);
        Serial.print(") using counter (");
        Serial.print(*counter_base);
        Serial.println(")");

      // Mode 0 (normal), using selected prescaler
      timerSetModePrescaler(timer, 0, prescaler + 1, *counter_base);
      return;
    }
  }
  
  // cannot find a suitable prescaler
  fatal("Cannot find a suitable prescaler :(");
}

unsigned lat;

///////////////////////////////////////////////////////////////////////////////
// BOOSTER CONFIG
///////////////////////////////////////////////////////////////////////////////

#define BOOSTER_STATUS_OK     0
#define BOOSTER_STATUS_OVP    1
#define BOOSTER_STATUS_HEAT   2

//   - Pins 5 and 6 are paired on timer0
//   - Pins 9 and 10 are paired on timer1
//   - Pins 3 and 11 are paired on timer2
struct booster_struct {
  char          *name;
  unsigned char  pwmSignalPin;
  unsigned char  dirSignalPin;
  unsigned char  tmpAlarmPin;  // temperature alarm
  unsigned char  ocpAlarmPin;  // over current protection alarm
  unsigned char  status;
} booster[] = {
    { "booster#1",  3,  2,   4,  5, BOOSTER_STATUS_OK },
    { "booster#2",  9,  6,   7,  8, BOOSTER_STATUS_OK },
    { "booster#3", 10, 11,  12, 13, BOOSTER_STATUS_OK },
//  { "booster#4", xx, xx,  xx, xx, BOOSTER_STATUS_OK },
};

#define BOOSTER_N ((sizeof(booster)) / (sizeof(struct booster_struct)))

void boosterSetup(void)
{
  Serial.print("Declared ");
  Serial.print(BOOSTER_N);
  Serial.println(" boosters.");
  for(int i = 0; i < BOOSTER_N; i++) {
    Serial.print("  - Configuring booster #");
    Serial.print(i);
    Serial.print(" (");
    Serial.print(booster[i].name);
    Serial.println(")");

    pinMode(booster[i].pwmSignalPin, OUTPUT); digitalWrite(booster[i].pwmSignalPin, LOW);
    pinMode(booster[i].dirSignalPin, OUTPUT); digitalWrite(booster[i].dirSignalPin, LOW);
    pinMode(booster[i].tmpAlarmPin, INPUT);
    pinMode(booster[i].ocpAlarmPin, INPUT);
  }
}

void boosterEmergencyStop(int b)
{
  if(b > (int) BOOSTER_N)
    fatal("Out of bounds booster.");

  if(b < 0) {
    Serial.println("Booster emergency stop!");
    for(int i = 0; i < BOOSTER_N; i++)
      boosterEmergencyStop(i);
  } else {
    digitalWrite(booster[b].pwmSignalPin, LOW);
    digitalWrite(booster[b].dirSignalPin, LOW);
  }
}

///////////////////////////////////////////////////////////////////////////////
// PWM
///////////////////////////////////////////////////////////////////////////////

struct pwmOutput_struct {
  int           pwmValTarget;
  int           pwmValCurrent;
  int           pwmAcc;
  unsigned char pwmMode;
} pwmOutput[BOOSTER_N];

#define PWM_OUTPUT_MODE_DIRECT    0
#define PWM_OUTPUT_MODE_INERTIAL  1
#define PWM_OUTPUT_N              BOOSTER_N
#define PWM_OUTPUT_MIN            60
#define PWM_OUTPUT_MAX            (255 - PWM_OUTPUT_MIN)
#define PWM_OUTPUT_ACCEL          7
#define PWM_OUTPUT_MAX_ACCEL      40

void pwmSetup(void)
{
  unsigned char sreg;
  
  //-------------------------------------------
  // CONFIGURE TIMERS
  disable_interrupts();

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
  
  //-------------------------------------------
  // CONFIGURE OUTPUT PINS FOR DIRECT PWM OUTPUT
  for(int b = 0; b < BOOSTER_N; b++) {
    switch(booster[b].pwmSignalPin) {
      case 9:
        TCCR1A |= 0b10000000; // Clear OCR1A on compare match
        break;
      case 10:
        TCCR1A |= 0b00100000; // Clear OCR1B on compare match
        break;
      case 11:
        TCCR2A |= 0b10000000; // Clear OCR2A on compare match
        break;
      case 3:
        TCCR2A |= 0b00100000; // Clear OCR2B on compare match
        break;
      default:
        fatal("Cannot configure output pin for PWM output");
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

///////////////////////////////////////////////////////////////////////////////
// DCC
///////////////////////////////////////////////////////////////////////////////

void dccSetup(void)
{
  unsigned char sreg;
  
  disable_interrupts();

  //-------------------------------------------
  // TURN OFF ALL TIMER 1 OUTPUTS
  TCCR1A = (TCCR1A & 0b00001100)
         | 0b00000000   // Normal mode (!WGM11, WGM10)
         | 0b00000000   // OC1A disconnected
         | 0b00000000;  // OC1B disconnected
  TCCR1B = (TCCR1B & 0b11100000)
         | 0x2          // clock prescaler 0x01 (16e6/8 = 2 Mhz)
         | 0b00000000;  // Normal mode (!WGM13, WGM12)
  OCR1A = OCR1B = 0;    // set compare registers to 0
  TIMSK1 &= 0b11011000; // disable all timer 1 interrupts
           
  //-------------------------------------------
  // ENABLE TIMER 2 OUTPUTS AS DCC OUTPUT (Pins 3 and 11)
  TCCR2A = (TCCR2A & 0b00001100)
         | 0b00000011;  // Fast PWM (WGM21, WGM20)
  TCCR2B = (TCCR2B & 0b11110000)
         | 0x6          // clock prescaler 0x6 (16e6/256 = 62.5 khz)
         | 0b00000000;  // Fast PWM (!WGM22)
  OCR2A = OCR2B = 0;    // set compare registers to 0
  TIMSK2 &= 0b11111000; // disable all timer 2 interrupts
  
  //-------------------------------------------
  // CONFIGURE OUTPUT PINS FOR DIRECT PWM OUTPUT
  for(int b = 0; b < BOOSTER_N; b++) {
    switch(booster[b].pwmSignalPin) {
      case 9:
        TCCR1A |= 0b10000000; // Clear OCR1A on compare match
        break;
      case 10:
        break;
      case 11:
        TCCR2A |= 0b10000000; // Clear OCR2A on compare match
        break;
      case 3:
        TCCR2A |= 0b00100000; // Clear OCR2B on compare match
        break;
      default:
        fatal("Cannot configure output pin for PWM output");
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

ISR(TIMER1_OVF_vect)
{
  // Capture the current timer value (TCTNx) (this is how much error we have
  // due to interrupt latency and the work in this function). Then add the
  // next scheduled action.
  // For more info, see http://www.uchobby.com/index.php/2007/11/24/arduino-interrupts/
  if(timerHandler) {
    unsigned x = timerHandler();
    lat = TCNT1;
    TCNT1 = lat + x;
  }
}

///////////////////////////////////////////////////////////////////////////////
// JOYSTICK
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

#define joyPressedUp()     joyPressed(JOY_UP)
#define joyPressedDown()   joyPressed(JOY_DOWN)
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

///////////////////////////////////////////////////////////////////////////////
// ANSI
///////////////////////////////////////////////////////////////////////////////

void ansi_cls(void)
{
  Serial.print("\x1B[1J");
  Serial.print("\x1B[1;1f");
}

void ansi_goto(int x, int y)
{
  Serial.print("\x1B[");
  Serial.print(y);
  Serial.print(';');
  Serial.print(x);
  Serial.print('f');
}

///////////////////////////////////////////////////////////////////////////////
// INTERFACE
///////////////////////////////////////////////////////////////////////////////

// GENERIC UI SCREEN
struct ui_screen {
  struct ui_screen *(*on_event)(struct ui_screen *ui, int event);
  int focus;
};

#define UI_EVENT_IDLE           0
#define UI_EVENT_OPEN           1
#define UI_EVENT_CLOSE          2
#define UI_EVENT_LEFT           11
#define UI_EVENT_RIGHT          12
#define UI_EVENT_UP             13
#define UI_EVENT_DOWN           14
#define UI_EVENT_PRESSED_LEFT   15
#define UI_EVENT_PRESSED_RIGHT  16
#define UI_EVENT_PRESSED_UP     17
#define UI_EVENT_PRESSED_DOWN   18
#define UI_EVENT_SELECT         20

// GLOBAL HANDLER
struct ui_screen *ui_curr = NULL;

void uiHandler(void)
{
  struct ui_screen *next_ui;
  
  // open UI if not opened yet
  if(!ui_curr->focus) {
    next_ui = ui_curr->on_event(ui_curr, UI_EVENT_OPEN);
    ui_curr->focus = 1;
    if(next_ui)
      fatal("ui_handler: fast close");
  }

  // get joystick events
  joyRead();
  if(joyPressedButton())     next_ui = ui_curr->on_event(ui_curr, UI_EVENT_SELECT);
  else if(joyPressedUp())    next_ui = ui_curr->on_event(ui_curr, UI_EVENT_PRESSED_UP);
  else if(joyPressedDown())  next_ui = ui_curr->on_event(ui_curr, UI_EVENT_PRESSED_DOWN);
  else if(joyPressedLeft())  next_ui = ui_curr->on_event(ui_curr, UI_EVENT_PRESSED_LEFT);
  else if(joyPressedRight()) next_ui = ui_curr->on_event(ui_curr, UI_EVENT_PRESSED_RIGHT);
  else if(joyMoveUp())       next_ui = ui_curr->on_event(ui_curr, UI_EVENT_UP);
  else if(joyMoveDown())     next_ui = ui_curr->on_event(ui_curr, UI_EVENT_DOWN);
  else if(joyMoveLeft())     next_ui = ui_curr->on_event(ui_curr, UI_EVENT_LEFT);
  else if(joyMoveRight())    next_ui = ui_curr->on_event(ui_curr, UI_EVENT_RIGHT);
  else
    next_ui = ui_curr->on_event(ui_curr, UI_EVENT_IDLE);
  
  // ignore keys
  while(Serial.available() > 0) {
    Serial.print(Serial.read());
  }
  
  if(next_ui && next_ui != ui_curr) {
    if(ui_curr->on_event(ui_curr, UI_EVENT_CLOSE))
      fatal("handle_ui: close event cannot set next_ui");
    ui_curr->focus = 0;
    ui_curr = next_ui;
  }
}

// GLOBAL OPTIONS SCREEN
struct ui_config_global_struct {
  ui_screen base;
  int       current;
  int       mode_pwm;
} ui_config_global = { { (ui_screen *(*)(ui_screen *, int)) ui_config_global_evh, 0 }, 0 };

// PWM SCREEN
struct ui_pwm_struct {
  ui_screen base;
  int       current_pwm;
} ui_pwm = { { (ui_screen *(*)(ui_screen *, int)) ui_pwm_evh, 0 }, 0 };


void ui_config_global_draw(struct ui_config_global_struct *ui, int cls)
{
  int i;
  
  if(cls) {
    ansi_cls();
    Serial.print("CONFIG\r\n======\r\n\r\n");
  }
  ansi_goto(1, 4);

  Serial.print(ui->current == 0 ? ">> " : "   ");
  Serial.print("Signal mode\t<");
  Serial.print(ui->mode_pwm ? "PWM" : "DCC");
  Serial.print(">   \r\n");
  Serial.print("\r\n");
  Serial.print(ui->current == 1 ? ">> " : "   ");
  Serial.print("RETURN");
  Serial.print("\r\n");
}

struct ui_screen *ui_config_global_evh(struct ui_config_global_struct *ui, int event)
{
  switch(event) {
    case UI_EVENT_PRESSED_UP:
      if(ui->current > 0) {
        ui->current--;
        ui_config_global_draw(ui, 0);
      }
      break;
      
    case UI_EVENT_PRESSED_DOWN:
      if(ui->current < 1) {
        ui->current++;
        ui_config_global_draw(ui, 0);
      }
      break;
      
    case UI_EVENT_PRESSED_LEFT:
    case UI_EVENT_PRESSED_RIGHT:
    case UI_EVENT_SELECT:
      switch(ui->current) {
        case 0:
          ui->mode_pwm = !ui->mode_pwm;
          break;
        
        case 1:
          if(event != UI_EVENT_SELECT)
            break;
          return ui->mode_pwm ? (struct ui_screen *) &ui_pwm : NULL;
      }
      ui_config_global_draw(ui, 0);
      break;
        
    case UI_EVENT_OPEN:
      ui_config_global_draw(ui, 1);
      break;
      
    case UI_EVENT_CLOSE:
      ansi_cls();
      break;
  }
  
  
  
  return NULL;
}

void ui_pwm_draw(struct ui_pwm_struct *ui, int cls)
{
  int i;
  
  if(cls) {
    ansi_cls();
    Serial.print("PWM POWER\r\n=========\r\n\r\n");
  }
  ansi_goto(1, 4);

  for(i = 0; i < BOOSTER_N; i++) {
    Serial.print(ui->current_pwm == i ? ">> " : "   ");
    Serial.print(booster[i].name);
    Serial.print("\t<");
    Serial.print(pwmOutput[i].pwmValCurrent);
    Serial.print(">   \r\n");
  }
  Serial.print("\r\n");
  Serial.print(ui->current_pwm == i ? ">> " : "   ");
  Serial.print("CONFIG MENU");
  Serial.print("\r\n");
}

struct ui_screen *ui_pwm_evh(struct ui_pwm_struct *ui, int event)
{
  switch(event) {
    case UI_EVENT_UP:
      if(ui->current_pwm < BOOSTER_N)
        pwmAccelerate(ui->current_pwm, 10);
      break;
      
    case UI_EVENT_DOWN:
      if(ui->current_pwm < BOOSTER_N)
        pwmAccelerate(ui->current_pwm, -10);
      break;
      
    case UI_EVENT_PRESSED_LEFT:
      if(ui->current_pwm > 0)
        ui->current_pwm--;
      break;
      
    case UI_EVENT_PRESSED_RIGHT:
      if(ui->current_pwm < BOOSTER_N)
        ui->current_pwm++;
      break;

    case UI_EVENT_SELECT:
      if(ui->current_pwm < BOOSTER_N)
        pwmStop(ui->current_pwm);
      else
        return (struct ui_screen *) &ui_config_global;
      break;
        
    case UI_EVENT_OPEN:
      ui_pwm_draw(ui, 1);
      break;
      
    case UI_EVENT_CLOSE:
      ansi_cls();
      break;
  }
  
  ui_pwm_draw(ui, 0);
  
  return NULL;
}

void setup()
{
  int i;
  Serial.begin(9600);
  Serial.println("Initializing");
  
  boosterSetup();
  joySetup();
  pwmSetup();
  
  ui_curr = (struct ui_screen *) &ui_pwm;
}


void loop()
{
  uiHandler();
  pwmRefresh();
  
  delay(100);
}

