///////////////////////////////////////////////////////////////////////////////
// DCC
///////////////////////////////////////////////////////////////////////////////

#include "interrupts.h"
#include "Arduino.h"
#include "USBAPI.h"
#include "dcc.h"
#include "booster.h"
#include "error.h"
#include "dcc.h"

struct booster_mngr_struct dccBoosterMngr =
	{ "DCC", dccStart, dccStop, dccRefresh };

#define DCC_CTC_ZERO    232
#define DCC_CTC_ONE     116

#define DCC_BUFFER_POOL_BITS  3
#define DCC_BUFFER_POOL_MASK  ((char) ~(0xff << DCC_BUFFER_POOL_BITS))
#define DCC_BUFFER_POOL_SIZE  (1 << DCC_BUFFER_POOL_BITS)

struct dcc_buffer_struct dcc_buffer_pool[DCC_BUFFER_POOL_SIZE];
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
    boosterDirSet(i, dccZero);
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
    boosterPwmSet(b, 1);

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
//ansi_goto(1,20);
//  Serial.print("msg_pending="); Serial.print((int) msg_pending);
//  Serial.print("  \r\nmsg=");   Serial.print((int) msg);
//  Serial.print("  \r\nlat=");   Serial.print((int) lat);
//  Serial.print("  \r\ndcc_last_msg_id="); Serial.print((int) dcc_last_msg_id);
//  Serial.print("    \r\n\r\n");
//  
//for(int i = 0; i < DCC_BUFFER_POOL_SIZE; i++) {
//  Serial.print("slot["); Serial.print(i); Serial.print("] = ");
//  Serial.print((int) dcc_buffer_pool[i].address); Serial.print("; ");
//  Serial.print((int) dcc_buffer_pool[i].reps);    Serial.println("   ");
//}
      
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
    Serial.println("!       ");
  }
}

