#include <avr/interrupt.h>

///////////////////////////////////////////////////////////////////////////////
// ERROR HANDLING
///////////////////////////////////////////////////////////////////////////////

void fatal(char *msg)
{
  boosterEmergencyStop();
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

struct booster_mngr_struct {
  char *name;
  void (*init)(void);
  void (*fini)(void);
  void (*refresh)(void);
} booster_mngr[] = {
  { "off", offStart, offStop, offRefresh },
  { "PWM", pwmStart, pwmStop, pwmRefresh },
  { "DCC", dccStart, dccStop, dccRefresh },
};
int booster_mngr_selected = 0;

#define BOOSTER_N      ((sizeof(booster)) / (sizeof(struct booster_struct)))
#define BOOSTER_MNGR_N ((sizeof(booster_mngr)) / (sizeof(struct booster_mngr_struct)))

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

void boosterEmergencyStop()
{
  Serial.println("Booster emergency stop!");
  
  // finish current booster manager
  booster_mngr[booster_mngr_selected].fini();
  
  // select OFF
  booster_mngr_selected = 0;
  booster_mngr[booster_mngr_selected].init();
}

///////////////////////////////////////////////////////////////////////////////
// OFF
///////////////////////////////////////////////////////////////////////////////

void offStart(void)
{
  for(int b = 0; b < BOOSTER_N; b++) {
    digitalWrite(booster[b].pwmSignalPin, LOW);
    digitalWrite(booster[b].dirSignalPin, LOW);
  }
}

void offStop(void)
{
}

void offRefresh(void)
{
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

#define DCC_CTC_ZERO    232
#define DCC_CTC_ONE     116

#define DCC_MSG_MAX           5
#define DCC_BUFFER_POOL_BITS  3
#define DCC_BUFFER_POOL_MASK  ((char) ~(0xff << DCC_BUFFER_POOL_BITS))
#define DCC_BUFFER_POOL_SIZE  (1 << DCC_BUFFER_POOL_BITS)

struct dcc_buffer_struct {
  byte msg[DCC_MSG_MAX];
  unsigned int  address;
//unsigned int command;
  unsigned char len;
  char reps;
} dcc_buffer_pool[DCC_BUFFER_POOL_SIZE];
byte dcc_msg_idle[] = { 0xff, 0x00, 0xff };

struct dcc_buffer_struct *dccSendBuffer(byte *msg, unsigned char len)
{
  unsigned int address, command;
  byte   x;
  struct dcc_buffer_struct *selected_buffer;
  
  if(len < 2)
    fatal("Message too short");
  if(len >= DCC_MSG_MAX)
    fatal("Message too long");
  
  // extract address
  address = msg[0]; // enough for 7-bit address, or broadcast (0x00) or idle packets (0xff)
  if(address & 0x80 && address != 0xff) {
    // yep! two-byte address
    if(len <= 2)
      fatal("Message too short (2 byte address but packet shorter)");
    address = address << 8 | msg[1];
  }
  Serial.print("address=");Serial.println(address);
  
  // extract command
  //command = *n & 0b11100000;
  //if(command == 0b01100000)
  //  command = 0b11000000;
  
  // search free buffer and kill other related buffers
  selected_buffer = NULL;
  for(char i = 0; i < DCC_BUFFER_POOL_SIZE; i++) {
    if(address == dcc_buffer_pool[i].address
    || !dcc_buffer_pool[i].address) {
      dcc_buffer_pool[i].reps = 0;
      dcc_buffer_pool[i].address = 0xffff;
    }
    if(!selected_buffer && dcc_buffer_pool[i].reps < 0) {
      selected_buffer = dcc_buffer_pool + i;
    }
  }
  //Serial.print("selected_buffer_id=");Serial.println(i);
  Serial.print("selected_buffer=");Serial.println((int) selected_buffer);
  if(!selected_buffer)
    return NULL;
  
  // copy msg to selected buffer
  x = 0;
  for(unsigned char i = 0; i < len; i++)
    x ^= selected_buffer->msg[i] = msg[i];
  selected_buffer->msg[len] = x;
  selected_buffer->len = len + 1;
//selected_buffer->command = command;
  selected_buffer->address = address;
  selected_buffer->reps = 100;
  
  return selected_buffer;
}

unsigned int lat;
unsigned dcc_excesive_lat;
int dcc_last_msg_id;
char msg_pending;
byte *msg = NULL;
#define DCC_STATE_PREAMBLE 0
ISR(TIMER1_COMPA_vect)
{
  static char dccZero = 0;
  static unsigned char dccCurrentBit;

  struct dcc_buffer_struct *cmsg;

  /* invert signal */
  for(int i = 0; i < BOOSTER_N; i++)
    digitalWrite(booster[i].dirSignalPin, dccZero);
  if((dccZero = !dccZero))
    return;

  if(!msg) {
    // we are still in preamble
    OCR1A = DCC_CTC_ONE;
    if(--dccCurrentBit > 0)
        goto check_latency;

    // select next msg
    dcc_last_msg_id = (dcc_last_msg_id + 1) & DCC_BUFFER_POOL_MASK;
    cmsg = dcc_buffer_pool + dcc_last_msg_id;
    if(cmsg->reps == 0) 
      cmsg->reps = -1;
    if(cmsg->reps > 0) {
      msg         = cmsg->msg;
      msg_pending = cmsg->len;
      cmsg->reps--;
    } else {
      msg         = dcc_msg_idle;
      dcc_msg_idle[3] = dcc_msg_idle[0] ^ dcc_msg_idle[1]  ^ dcc_msg_idle[2];
      msg_pending = sizeof(dcc_msg_idle);
    }
    dccCurrentBit = 0;
  } else {
    if(dccCurrentBit) {
      // send data bit
      OCR1A = (*msg & dccCurrentBit) ? DCC_CTC_ONE : DCC_CTC_ZERO;
      dccCurrentBit >>= 1;
      if(!dccCurrentBit) {
        msg++;
        msg_pending--;
      }
    } else {
      // separator bit
      dccCurrentBit = 0x80;
      if(msg_pending <= 0) {
        msg = NULL;
        dccCurrentBit = 16;
        OCR1A = DCC_CTC_ONE; // last bit
      } else {
        OCR1A = DCC_CTC_ZERO; // more data will come
      }
    }
  }

check_latency:
  // Capture the current timer value (TCTNx) for debugging purposes. It always
  // should be below 116 (a DCC one). Elsewhere the DCC generator will produce
  // corrupt signals :P
  //unsigned int 
  lat = TCNT1;
  if(lat >= DCC_CTC_ONE)
    dcc_excesive_lat = lat;
}

void dccStart(void)
{
  unsigned char sreg;
  
  disable_interrupts();

  // CONFIGURE TIMER 1
  TCCR1A = (TCCR1A & 0b00001100)
         | 0b00000000     // CTC  mode (!WGM11, !WGM10)
         | 0b00000000     // OC1A disconnected
         | 0b00000000;    // OC1B disconnected
  TCCR1B = (TCCR1B & 0b11100000)
         | 0x2            // clock prescaler 0x02 (16e6/8 = 2 Mhz)
         | 0b00001000;    // Normal mode (!WGM13, WGM12)
  OCR1B = 0;      // set compare registers to 0
  OCR1A = DCC_CTC_ZERO;
  TIMSK1 = (TIMSK1 & 0b11011000)
         | (1 << OCIE1A); // output cmp A match interrupt enable
           
  // DISABLE TIMER 2
  TCCR2A = TCCR2A & 0b00001100;
  TCCR2B = TCCR2B & 0b11110000;
  OCR2A = OCR2B = 0;
  TIMSK2 &= 0b11111000;

  // SET PWM OUTPUTS TO 1
  for(int b = 0; b < BOOSTER_N; b++)
    digitalWrite(booster[b].pwmSignalPin, 1);

  //-------------------------------------------
  // RESET DCC STATUS
  memset(dcc_buffer_pool, 0, sizeof(dcc_buffer_pool));

  enable_interrupts();
}

void dccStop(void)
{
  disable_interrupts();

  // disconnect all OC1A:B/OC2A:B pins and go to normal operations mode
  TCCR1A = TCCR1A & 0b00001100;    TCCR2A = TCCR2A & 0b00001100;
  TCCR1B = TCCR1B & 0b11100000;    TCCR2B = TCCR2B & 0b11110000;
  OCR1A = OCR1B = 0;               OCR2A = OCR2B = 0;
  TIMSK1 &= 0b11011000;            TIMSK2 &= 0b11111000;

  enable_interrupts();
}

unsigned int lolol = 0;
unsigned int dir = 0;
void dccRefresh(void)
{
  ansi_goto(1,20);
    Serial.print("msg_pending="); Serial.print((int) msg_pending);
    Serial.print("  \r\nmsg=");   Serial.print((int) msg);
    Serial.print("  \r\nlat=");   Serial.print((int) lat);
    Serial.print("  \r\ndcc_last_msg_id="); Serial.print((int) dcc_last_msg_id);
    Serial.print("    \r\n\r\n");
    
  for(int i = 0; i < DCC_BUFFER_POOL_SIZE; i++) {
    Serial.print("slot["); Serial.print(i); Serial.print("] = ");
    Serial.print((int) dcc_buffer_pool[i].address); Serial.print("; ");
    Serial.print((int) dcc_buffer_pool[i].reps);    Serial.println("   ");
  }
      
  if(lolol++ >= 20) {
    Serial.println("YEP!");
    //byte msg[] = { 3, 0b10000000 };
    //msg[1] |= dir ? 0b10010000 : 0;
    byte msg[] = { 3, 0b01001000 };
    msg[1] |= dir ? 0b00100000 : 0;
    dir = !dir;
    lolol = 0;
    struct dcc_buffer_struct *b = dccSendBuffer(msg, sizeof(msg));
    if(b) {
    Serial.print("\r\nmsg.len = "); Serial.print(b->len); Serial.print("     ");
    for(unsigned char i = 0; i < b->len; i++) {
      Serial.print("\r\nmsg[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.print(b->msg[i]);
      Serial.print("    ");
    }
    } else {
      for(int i = 0; i < 5; i++)
      Serial.println("                                              ");
    }
  } else {
    Serial.print("----");
  }

  if(dcc_excesive_lat) {
    Serial.print("dcc_excesive_lat = ");
    Serial.println(dcc_excesive_lat);
    Serial.println("!!!       ");
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
  static int last_key = 0;
  int event, k;
  
  // open UI if not opened yet
  if(!ui_curr->focus) {
    next_ui = ui_curr->on_event(ui_curr, UI_EVENT_OPEN);
    ui_curr->focus = 1;
    if(next_ui)
      fatal("ui_handler: fast close");
  }

  // get joystick events
  joyRead();
  event = UI_EVENT_IDLE;
  if(joyPressedButton())     event = UI_EVENT_SELECT;
  else if(joyPressedUp())    event = UI_EVENT_PRESSED_UP;
  else if(joyPressedDown())  event = UI_EVENT_PRESSED_DOWN;
  else if(joyPressedLeft())  event = UI_EVENT_PRESSED_LEFT;
  else if(joyPressedRight()) event = UI_EVENT_PRESSED_RIGHT;
  else if(joyMoveUp())       event = UI_EVENT_UP;
  else if(joyMoveDown())     event = UI_EVENT_DOWN;
  else if(joyMoveLeft())     event = UI_EVENT_LEFT;
  else if(joyMoveRight())    event = UI_EVENT_RIGHT;
    
  // get keys
  k = 0;
  if(Serial.available() > 0) {
    int c;
    if((c = Serial.read()) == 27
    && (c = Serial.read()) == 91)
    { // escape char
      k = Serial.read() | 0x1000;
      switch(k) {
        case 66 | 0x1000: event = last_key == k ? UI_EVENT_DOWN  : UI_EVENT_PRESSED_DOWN;  break;
        case 65 | 0x1000: event = last_key == k ? UI_EVENT_UP    : UI_EVENT_PRESSED_UP;    break;
        case 68 | 0x1000: event = last_key == k ? UI_EVENT_LEFT  : UI_EVENT_PRESSED_LEFT;  break;
        case 67 | 0x1000: event = last_key == k ? UI_EVENT_RIGHT : UI_EVENT_PRESSED_RIGHT; break;
        //default:
          //Serial.print("escape => "); Serial.print(k & 0xff);
      }
    } else {
      if(c == 13) {
        event = UI_EVENT_SELECT;
      }
    }
  }
  last_key = k;
  
  next_ui = ui_curr->on_event(ui_curr, event);
  
  if(next_ui && next_ui != ui_curr) {
    if(ui_curr->on_event(ui_curr, UI_EVENT_CLOSE))
      fatal("handle_ui: close event cannot set next_ui");
    ui_curr->focus = 0;
    ui_curr = next_ui;
  }
}

// HELLO SCREEN
struct ui_hello_struct {
  ui_screen base;
  int       ticks_to_go;
} ui_hello = { { (ui_screen *(*)(ui_screen *, int)) ui_hello_evh, 0 }, 20 };

// GLOBAL OPTIONS SCREEN
struct ui_config_global_struct {
  ui_screen base;
  int       current;
} ui_config_global = { { (ui_screen *(*)(ui_screen *, int)) ui_config_global_evh, 0 }, 0 };

// PWM SCREEN
struct ui_pwm_struct {
  ui_screen base;
  int       current_pwm;
} ui_pwm = { { (ui_screen *(*)(ui_screen *, int)) ui_pwm_evh, 0 }, 0 };

// DCC SCREEN
//struct ui_pwm_struct {
//  ui_screen base;
//  int       current_pwm;
//} ui_pwm = { { (ui_screen *(*)(ui_screen *, int)) ui_pwm_evh, 0 }, 0 };

PROGMEM prog_char ui_hello_str_0[]  = "  +--- DUAL PWM/DCC CONTROLLER v1.0 ------------------------------------+";
PROGMEM prog_char ui_hello_str_1[]  = "  |                                                                     |";
PROGMEM prog_char ui_hello_str_2[]  = "  |  PPPPPP  WW         WW MM       MM    // DDDDDD     CCCCCC  CCCCCC  |";
PROGMEM prog_char ui_hello_str_3[]  = "  |  PP   PP WW         WW MMMM   MMMM   //  DD   DD   CC      CC       |";
PROGMEM prog_char ui_hello_str_4[]  = "  |  PP   PP WW    W    WW MM MM MM MM   //  DD    DD CC      CC        |";
PROGMEM prog_char ui_hello_str_5[]  = "  |  PPPPPP  WW   WWW   WW MM  MMM  MM   //  DD    DD CC      CC        |";
PROGMEM prog_char ui_hello_str_6[]  = "  |  PP       WW WW WW WW  MM   M   MM  //   DD    DD CC      CC        |";
PROGMEM prog_char ui_hello_str_7[]  = "  |  PP       WWWW  WWWW   MM       MM  //   DD   DD   CC      CC       |";
PROGMEM prog_char ui_hello_str_8[]  = "  |  PP        WW    WW    MM       MM //    DDDDDD     CCCCCC  CCCCCC  |";
PROGMEM prog_char ui_hello_str_9[]  = "  |                                                                     |";
PROGMEM prog_char ui_hello_str_10[] = "  +------------------------------------ DUAL PWM/DCC CONTROLLER v1.0 ---+";
PROGMEM prog_char ui_hello_str_11[] = "  (C) 2013 Gerardo García Peña <killabytenow@gmail.com>\r\n";
PROGMEM prog_char ui_hello_str_12[] = "  This program is free software; you can redistribute it and/or modify it";
PROGMEM prog_char ui_hello_str_13[] = "  under the terms of the GNU General Public License as published by the Free";
PROGMEM prog_char ui_hello_str_14[] = "  Software Foundation; either version 3 of the License, or (at your option)";
PROGMEM prog_char ui_hello_str_15[] = "  any later version.\r\n";
PROGMEM prog_char ui_hello_str_16[] = "  This program is distributed in the hope that it will be useful, but WITHOUT";
PROGMEM prog_char ui_hello_str_17[] = "  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or";
PROGMEM prog_char ui_hello_str_18[] = "  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for";
PROGMEM prog_char ui_hello_str_19[] = "  more details.\r\n";
PROGMEM prog_char ui_hello_str_20[] = "  You should have received a copy of the GNU General Public License along";
PROGMEM prog_char ui_hello_str_21[] = "  with this program; if not, write to the Free Software Foundation, Inc., 51";
PROGMEM prog_char ui_hello_str_22[] = "  Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA";

PROGMEM const char *ui_hello_str[] = {
  ui_hello_str_0,  ui_hello_str_1,  ui_hello_str_2,  ui_hello_str_3,  ui_hello_str_4,
  ui_hello_str_5,  ui_hello_str_6,  ui_hello_str_7,  ui_hello_str_8,  ui_hello_str_9,
  ui_hello_str_10, ui_hello_str_11, ui_hello_str_12, ui_hello_str_13, ui_hello_str_14,
  ui_hello_str_15, ui_hello_str_16, ui_hello_str_17, ui_hello_str_18, ui_hello_str_19,
  ui_hello_str_20, ui_hello_str_21, ui_hello_str_22,
};

void ui_hello_draw(void)
{
  char buffer[100];

  ansi_cls();
  ansi_goto(1, 5);
  for(int i = 0; i < sizeof(ui_hello_str) / sizeof(char *); i++) {
    strcpy_P(buffer, (char *) pgm_read_word(&(ui_hello_str[i])));
    Serial.println(buffer);
  }
}

struct ui_screen *ui_hello_evh(struct ui_hello_struct *ui, int event)
{
  if(event == UI_EVENT_OPEN)
    ui_hello_draw();
  if(event == UI_EVENT_CLOSE)
    return NULL;
    
  return --ui->ticks_to_go > 0 ? NULL : (struct ui_screen *) &ui_config_global;
}

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
  Serial.print(booster_mngr[booster_mngr_selected].name);
  Serial.print(">   \r\n");
  Serial.print("\r\n");
  Serial.print(ui->current == 1 ? ">> " : "   ");
  Serial.print("RETURN");
  Serial.print("\r\n");
}

struct ui_screen *ui_config_global_evh(struct ui_config_global_struct *ui, int event)
{
  int n;
  
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
          n = booster_mngr_selected + (event == UI_EVENT_PRESSED_LEFT ? -1 : 1);
          if(n < 0)
            n = BOOSTER_MNGR_N - 1;
          if(n >= BOOSTER_MNGR_N)
            n = 0;
          booster_mngr[booster_mngr_selected].fini();
          booster_mngr_selected = n;
          booster_mngr[booster_mngr_selected].init();
          break;
        
        case 1:
          if(event != UI_EVENT_SELECT)
            break;
          switch(booster_mngr_selected) {
            case 0: return NULL;
            case 1: return (struct ui_screen *) &ui_pwm;
            case 2: return NULL;
          }
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
    Serial.print(((float) pwmOutput[i].pwmValCurrent) * 100.0 / ((float) PWM_OUTPUT_MAX));
    Serial.print(">   \r\n");
  }
  Serial.print("\r\n");
  Serial.print(ui->current_pwm == i ? ">> " : "   ");
  Serial.print("CONFIG MENU");
  Serial.print("\r\n");
}

struct ui_screen *ui_pwm_evh(struct ui_pwm_struct *ui, int event)
{
  int redraw = 0;
  static int refresh = 0;
  
  switch(event) {
    case UI_EVENT_LEFT:
      if(ui->current_pwm < BOOSTER_N)
        pwmAccelerate(ui->current_pwm, -10);
      break;
      
    case UI_EVENT_RIGHT:
      if(ui->current_pwm < BOOSTER_N)
        pwmAccelerate(ui->current_pwm, 10);
      break;
      
    case UI_EVENT_PRESSED_UP:
      if(ui->current_pwm > 0) {
        ui->current_pwm--;
        redraw = 1;
      }
      break;
      
    case UI_EVENT_PRESSED_DOWN:
      if(ui->current_pwm < BOOSTER_N) {
        ui->current_pwm++;
        redraw = 1;
      }
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
  
  for(int i = 0; !redraw && i < BOOSTER_N; i++)
    redraw = pwmOutput[i].pwmValCurrent != pwmOutput[i].pwmValTarget;
  refresh++;
  redraw |= (refresh == 10);
  
  if(redraw) {
    refresh = 0;
    ui_pwm_draw(ui, 0);
  }

  return NULL;
}

void setup()
{
  int i;

  //delay(5000);
  
  //Serial.begin(9600);
  Serial.begin(115200);
  Serial.println("Initializing");
  
  boosterSetup();
  joySetup();
  
  ui_curr = (struct ui_screen *) &ui_hello;
}


void loop()
{
  //pwmRefresh();
  booster_mngr[booster_mngr_selected].refresh();
  uiHandler();
  
  delay(100);
}

