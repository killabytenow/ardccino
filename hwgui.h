/*****************************************************************************
 * hwgui.h
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

#ifndef __HWGUI_H__
#define __HWGUI_H__

#include "config.h"

#ifdef HWGUI_ENABLED

class UIScreen {
	static UIScreen *current;

protected:
	static uint16_t tft_xsize;
	static uint16_t tft_ysize;
	bool focus;
	virtual void draw(void) = 0;
	virtual void do_open_event(void) { draw(); };
	virtual void do_close_event(void) { };
	virtual UIScreen *do_joystick_event(void) { return NULL; };
	virtual UIScreen *do_tick_event(void) { return NULL; };

public:
	UIScreen();
	static void handle(void);
	static void init(UIScreen *start);
};

class UIHello : public UIScreen {
private:
	int       ticks_to_go;

protected:
	void draw(void);
	void do_open_event(void);
	UIScreen *do_tick_event(void);
};

class UIGlobalConfig : public UIScreen {
private:
	int c_opt;

protected:
	void draw(void);
	UIScreen *do_joystick_event(void);
};

class UIPWM : public UIScreen {
private:
	int c_opt;
	int refresh;

protected:
	void draw(void);
	UIScreen *do_joystick_event(void);
	UIScreen *do_tick_event(void);
};

extern UIHello ui_hello;
extern UIGlobalConfig ui_global_config;
extern UIPWM ui_pwm;

#endif

#endif
