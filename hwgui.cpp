#include <UTFT.h>
#include <Arduino.h>

#include "hwgui.h"
#include "booster.h"
#include "booster_mngr.h"
#include "error.h"
#include "tinyfont.h"

///////////////////////////////////////////////////////////////////////////////
// UTFT
///////////////////////////////////////////////////////////////////////////////

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];

uint16_t tft_xsize;
uint16_t tft_ysize;

#define TFT_SCK       44
#define TFT_SDI_MOSI  46
#define TFT_DC        48
#define TFT_RESET     50
#define TFT_CS        52

UTFT tft(TFT01_22SP, TFT_SDI_MOSI, TFT_SCK, TFT_CS, TFT_RESET, TFT_DC);

void utftSetup(void)
{
  tft.InitLCD();
  tft.setFont(TinyFont);
  tft_xsize = tft.getDisplayXSize();
  tft_ysize = tft.getDisplayYSize();
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
// INTERFACE
///////////////////////////////////////////////////////////////////////////////

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


PROGMEM const char ui_hello_str_0[]  = "+-- DUAL PWM/DCC CONTROLLER v1.0 -----+";
PROGMEM const char ui_hello_str_1[]  = "|                                     |";
PROGMEM const char ui_hello_str_2[]  = "| PPP  W   W M   M   / DDD   CCC  CCC |";
PROGMEM const char ui_hello_str_3[]  = "| P  P W   W MM MM   / D  D C    C    |";
PROGMEM const char ui_hello_str_4[]  = "| P  P W   W M M M  /  D  D C    C    |";
PROGMEM const char ui_hello_str_5[]  = "| PPP  W W W M M M  /  D  D C    C    |";
PROGMEM const char ui_hello_str_6[]  = "| P    W W W M   M  /  D  D C    C    |";
PROGMEM const char ui_hello_str_7[]  = "| P    WW WW M   M  /  D  D C    C    |";
PROGMEM const char ui_hello_str_8[]  = "| P    W   W M   M /   DDD   CCC  CCC |";
PROGMEM const char ui_hello_str_9[]  = "|                                     |";
PROGMEM const char ui_hello_str_10[] = "+-------------------------------------+";
PROGMEM const char ui_hello_str_11[] = "(C) 2013 Gerardo García Peña";
PROGMEM const char ui_hello_str_12[] = "              <killabytenow@gmail.com>";
PROGMEM const char ui_hello_str_13[] = "This program is free software; you can";
PROGMEM const char ui_hello_str_14[] = "redistribute it and/or modify it under";
PROGMEM const char ui_hello_str_15[] = "the terms of the GNU General Public License as published by the Free";
PROGMEM const char ui_hello_str_16[] = "Software Foundation; either version 3 of the License, or (at your option)";
PROGMEM const char ui_hello_str_17[] = "any later version.";
PROGMEM char const * const ui_hello_str[] = {
  ui_hello_str_0,  ui_hello_str_1,  ui_hello_str_2,  ui_hello_str_3,  ui_hello_str_4,
  ui_hello_str_5,  ui_hello_str_6,  ui_hello_str_7,  ui_hello_str_8,  ui_hello_str_9,
  ui_hello_str_10, ui_hello_str_11, ui_hello_str_12, ui_hello_str_13, ui_hello_str_14,
  ui_hello_str_15, ui_hello_str_16, ui_hello_str_17,
};


void ui_hello_draw(void)
{
  char buffer[100];

  Serial.println("drawing hello");

  tft.clrScr();
  tft.setFont(SmallFont);
  tft.setColor(255, 255, 255);
  tft.setBackColor(255, 0, 0);
  tft.print("DUAL PWM/DCC CONTROLLER v1.0", CENTER, 1);
  
  tft.setFont(TinyFont);
  tft.setColor(255, 255, 0);
  tft.setBackColor(0, 0, 255);
  int y = 12;
  for(int i = 0; i < sizeof(ui_hello_str) / sizeof(char *); i++) {
    strcpy_P(buffer, (char *) pgm_read_word(&(ui_hello_str[i])));
    tft.print(buffer, 1, y);
    y += 8;
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
	int x1, y1, x2, y2, l;

	tft.setBackColor(VGA_TRANSPARENT);
	if(cls) {
		tft.fillScr(VGA_NAVY);
		tft.setColor(VGA_MAROON);
		tft.drawRect(0, 0, tft_xsize, 12);
		tft.setColor(VGA_WHITE);
		tft.print("Select working mode", CENTER, 0);
	}

	// draw first mode (usually OFF)
	tft.fillScr(VGA_BLUE);
	y1 = tft_ysize / 16;
	y2 = tft_xsize * 5 / 16;
	x1 = tft_xsize / 16;
	x2 = tft_xsize * 15 / 32;
	tft.fillRoundRect(x1, y1, x2, y2);
	tft.setColor(VGA_GRAY);
	tft.drawRoundRect(x1, y1, x2, y2);
	l = strlen(booster_mngr[booster_mngr_selected].name);
	tft.print(booster_mngr[booster_mngr_selected].name,
			(x2 + x1 - l) >> 1, (y2 + y1 - 12) >> 1);

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
