/*****************************************************************************
 * hwgui.h
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

#ifndef __HWGUI_H__
#define __HWGUI_H__

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

extern struct ui_screen *ui_curr;

struct ui_screen *ui_config_global_evh(struct ui_config_global_struct *ui, int event);
struct ui_screen *ui_hello_evh(struct ui_hello_struct *ui, int event);
struct ui_screen *ui_pwm_evh(struct ui_pwm_struct *ui, int event);

#endif
