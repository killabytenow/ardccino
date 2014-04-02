/*****************************************************************************
 * hwgui.cpp
 *
 * Hardware graphical user interface. For use with LCD/TFT touch screens.
 *
 * ---------------------------------------------------------------------------
 * ardccino - Arduino dual PWM/DCC controller
 *   (C) 2013-2014 Gerardo García Peña <killabytenow@gmail.com>
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
#include "fonts.h"
#include "joystick.h"

#ifdef HWGUI_ENABLED

// following files are generated with 'gen_code.sh' script using the contents
// of 'banner.txt' file.
#include "auto_banner.h"

///////////////////////////////////////////////////////////////////////////////
// UTFT
///////////////////////////////////////////////////////////////////////////////

uint16_t UIScreen::tft_xsize;
uint16_t UIScreen::tft_ysize;

///////////////////////////////////////////////////////////////////////////////
// CREATE SCREENS
///////////////////////////////////////////////////////////////////////////////

UIHello        ui_hello         = UIHello();
UIGlobalConfig ui_global_config = UIGlobalConfig();
UIPWM          ui_pwm           = UIPWM();

///////////////////////////////////////////////////////////////////////////////
// ABSTRACT INTERFACE SCREEN
///////////////////////////////////////////////////////////////////////////////

UIScreen *UIScreen::current = NULL;

UIScreen::UIScreen()
{
	focus = false;
}

void UIScreen::init(UIScreen *start)
{
	tft.setFont(TinyFont);
	tft_xsize = tft.getDisplayXSize();
	tft_ysize = tft.getDisplayYSize();
	current = start;
}

void UIScreen::handle(void)
{
	UIScreen *next_ui;

	if(!current->focus) {
		current->do_open_event();
		current->focus = true;
	}

	Joystick::read();
	if((Joystick::now | Joystick::old)
	&& (next_ui = current->do_joystick_event()) != NULL)
		goto go_next_ui;
	
	if((next_ui = current->do_tick_event()) != NULL)
		goto go_next_ui;

	return;

go_next_ui:
	current->focus = false;
	current->do_close_event();
	current = next_ui;
}

///////////////////////////////////////////////////////////////////////////////
// HELLO SCREEN
///////////////////////////////////////////////////////////////////////////////

void UIHello::draw(void)
{
	char buffer[100];

	cli.debug("drawing hello");

	tft.fillScr(VGA_BLUE);
	tft.setFont(SmallFont);
	tft.setColor(255, 255, 255);
	tft.setBackColor(255, 0, 0);
	tft.print((char *) "DUAL PWM/DCC CONTROLLER v1.0", CENTER, 1);

	tft.setFont(TinyFont);
	tft.setColor(255, 255, 0);
	tft.setBackColor(0, 0, 255);
	int y = 12;
	for(unsigned i = 0; i < sizeof(banner) / sizeof(char *); i++) {
		strcpy_P(buffer, (char *) pgm_read_ptr(&(banner[i])));
		tft.print(buffer, 1, y);
		y += 8;
	}
}

void UIHello::do_open_event(void)
{
	ticks_to_go = 20;
	draw();
}


UIScreen *UIHello::do_tick_event(void)
{
	return --ticks_to_go > 0
		? NULL
		: &ui_global_config;
}

///////////////////////////////////////////////////////////////////////////////
// GLOBAL CONFIG SCREEN
///////////////////////////////////////////////////////////////////////////////

void UIGlobalConfig::draw(void)
{
	//int i;
	//int x1, y1, x2, y2;

	tft.setBackColor(VGA_TRANSPARENT);
	if(!focus) {
		tft.fillScr(VGA_NAVY);
		tft.setColor(VGA_MAROON);
		//tft.drawRect(0, 0, tft_xsize, 12);
		tft.setColor(VGA_WHITE);
		tft.print((char *) "Select working mode", CENTER, 0);
	}

	// draw first mode (usually OFF)
	//tft.fillScr(VGA_BLUE);
	//y1 = tft_ysize / 16;
	//y2 = tft_xsize * 5 / 16;
	//x1 = tft_xsize / 16;
	//x2 = tft_xsize * 15 / 32;
	//tft.fillRoundRect(x1, y1, x2, y2);
	//tft.setColor(VGA_GRAY);
	//tft.drawRoundRect(x1, y1, x2, y2);
	//tft.print((char *) "Off", (x2 + x1 - 3*8) >> 1, (y2 + y1 - 12) >> 1);

	//Serial.print(current == 0 ? ">> " : "   ");
	//Serial.print("Signal mode\t<");
	//Serial.print(booster_mngr[booster_mngr_selected].name);
	//Serial.print(">   \r\n");
	//Serial.print("\r\n");
	//Serial.print(current == 1 ? ">> " : "   ");
	//Serial.print("RETURN");
	//Serial.print("\r\n");
}

UIScreen *UIGlobalConfig::do_joystick_event(void)
{
	if(Joystick::pressed(JOY_UP)) {
		if(c_opt == 0)
			return NULL;
		c_opt--;
	} else
	if(Joystick::pressed(JOY_DOWN)) {
		if(c_opt == 1)
			return NULL;
		c_opt++;
	} else
	if(Joystick::pressed(JOY_BUTTON)) {
		switch(c_opt) {
		case 0: off.enable(); break;
		case 1: pwm.enable(); return &ui_pwm;
		case 2: dcc.enable(); break;
		default:
			cli.fatal("Unknown option.");
		}
	}
	draw();

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// PWM SCREEN
///////////////////////////////////////////////////////////////////////////////

void UIPWM::draw(void)
{
	//int i;

	refresh = 0;
	if(!focus) {
		tft.fillScr(VGA_NAVY);
		Serial.print("PWM POWER\r\n=========\r\n\r\n");
	}
	//ansi_goto(1, 4);

	//for(i = 0; i < BOOSTER_N; i++) {
	//	Serial.print(current_pwm == i ? ">> " : "   ");
	//	Serial.print(booster[i].name);
	//	Serial.print("\t<");
	//	Serial.print(((float) pwmOutput[i].pwmValCurrent) * 100.0 / ((float) PWM_OUTPUT_MAX));
	//	Serial.print(">   \r\n");
	//}
	//Serial.print("\r\n");
	//Serial.print(current_pwm == i ? ">> " : "   ");
	//Serial.print("CONFIG MENU");
	//Serial.print("\r\n");
}

UIScreen *UIPWM::do_joystick_event(void)
{
	if(Joystick::move(JOY_LEFT)) {
		if(c_opt >= pwm.nboosters)
			return NULL;
		pwm.accelerate(c_opt, -10);
	} else
	if(Joystick::move(JOY_RIGHT)) {
		if(c_opt >= pwm.nboosters)
			return NULL;
		pwm.accelerate(c_opt, 10);
	} else
	if(Joystick::pressed(JOY_UP)) {
		if(c_opt == 0)
			return NULL;
		c_opt--;
	} else
	if(Joystick::pressed(JOY_DOWN)) {
		if(c_opt >= pwm.nboosters)
			return NULL;
		c_opt++;
	} else
	if(Joystick::pressed(JOY_BUTTON)) {
		if(c_opt < pwm.nboosters)
			pwm.stop(c_opt);
		else
			return &ui_global_config;
	}

	return NULL;
}

UIScreen *UIPWM::do_tick_event(void)
{
	bool redraw = false;
	for(int i = 0; !redraw && i < pwm.nboosters; i++)
		redraw = pwm.boosters[i].curr_power != pwm.boosters[i].trgt_power;
	refresh++;
	if(redraw || refresh > 10)
		draw();

	return NULL;
}

#endif
