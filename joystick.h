/*****************************************************************************
 * joystick.h
 *
 * Joystick handler
 *
 * ---------------------------------------------------------------------------
 * ardccino - Arduino dual PWM/DCC controller
 *   (C) 2013-2015 Gerardo García Peña <killabytenow@gmail.com>
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

#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#include "config.h"

#ifdef JOY_ENABLED

#define JOY_UP     0x01
#define JOY_DOWN   0x02
#define JOY_LEFT   0x04
#define JOY_RIGHT  0x08
#define JOY_BUTTON 0x10

class Joystick {
	public:
		static uint8_t now;
		static uint8_t old;

		static void read(void);
		static bool move(uint8_t flag);
		static bool pressed(uint8_t flag);
		static void print();
};

#ifdef SIMULATOR
extern int sim_joystick_status;
#endif

#endif

#endif
