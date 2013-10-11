/*****************************************************************************
 * hwgui.cpp
 *
 * Hardware graphical user interface. For use with LCD/TFT touch screens.
 *
 * ---------------------------------------------------------------------------
 * ardccino - Arduino dual PWM/DCC controller
 *   (C) 2013 Gerardo García Peña <killabytenow@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 3 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 *****************************************************************************/

#include "config.h"
#include "hwgui.h"
#include "booster.h"
#include "booster_mngr.h"
#include "error.h"
#include "fonts.h"

#ifdef HWGUI_ENABLE

// following files are generated with 'gen_code.sh' script using the contents
// of 'banner.txt' file.
#include ".banner.h"

///////////////////////////////////////////////////////////////////////////////
// UTFT
///////////////////////////////////////////////////////////////////////////////

uint16_t tft_xsize;
uint16_t tft_ysize;

void utftSetup(void)
{
  tft.setFont(TinyFont);
  tft_xsize = tft.getDisplayXSize();
  tft_ysize = tft.getDisplayYSize();
}

///////////////////////////////////////////////////////////////////////////////
// JOYSTICK
///////////////////////////////////////////////////////////////////////////////

#define joyMoveUp()        joyMove(JOY_UP)
#define joyMoveDown()      joyMove(JOY_DOWN)
#define joyMoveLeft()      joyMove(JOY_LEFT)
#define joyMoveRight()     joyMove(JOY_RIGHT)

#define joyPressedUp()     joyPressed(JOY_UP)
#define joyPressedDown()   joyPressed(JOY_DOWN)
#define joyPressedLeft()   joyPressed(JOY_LEFT)
#define joyPressedRight()  joyPressed(JOY_RIGHT)
#define joyPressedButton() joyPressed(JOY_BUTTON)

///////////////////////////////////////////////////////////////////////////////
// INTERFACE
///////////////////////////////////////////////////////////////////////////////

// GLOBAL HANDLER
void UIScreen::handle(void)
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
  for(int i = 0; i < sizeof(banner) / sizeof(char *); i++) {
    strcpy_P(buffer, (char *) pgm_read_word(&(banner[i])));
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
	tft.print("Off", (x2 + x1 - 3*8) >> 1, (y2 + y1 - 12) >> 1);

	//Serial.print(ui->current == 0 ? ">> " : "   ");
	//Serial.print("Signal mode\t<");
	//Serial.print(booster_mngr[booster_mngr_selected].name);
	//Serial.print(">   \r\n");
	//Serial.print("\r\n");
	//Serial.print(ui->current == 1 ? ">> " : "   ");
	//Serial.print("RETURN");
	//Serial.print("\r\n");
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

#endif
