/*****************************************************************************
 * ardccino.ino
 *
 * Setup and main loop
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

#include <memorysaver.h>

#include "booster.h"
#include "dcc.h"

//   - Pins 5 and 6 are paired on timer0
//   - Pins 9 and 10 are paired on timer1
//   - Pins 3 and 11 are paired on timer2
Booster *boosters[BOOSTERS_N] = {
	Booster("booster#1",  3,  2,   4,  5, 14),
	Booster("booster#2",  9,  6,   7,  8, 14),
	Booster("booster#3", 10, 11,  12, 13, 14),
//	Booster("service",   xx, xx,  xx, xx, xx),
};

OffMngr off(boosters, 3);
PwmMngr pwm(boosters, 3);
DccMngr dcc(boosters, 3, -1);
//DccMngr dcc(boosters, 2, 1);

void setup()
{
	int i;

	//Serial.begin(9600);
	Serial.begin(115200);
	Serial.println("Initializing");

	joySetup();
	utftSetup();
  
	// init menu ui
	//ui_curr = (struct ui_screen *) &ui_hello;
	//current_ui_handler = uiHandler;
	current_ui_handler = cliHandler;
}

void loop()
{
	BoosterMngr::refresh_current();
	current_ui_handler();
  
	delay(100);
}

