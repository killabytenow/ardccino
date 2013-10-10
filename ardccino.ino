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

#define __DECLARE_GLOBALS__ 1
#include "config.h"
#define BOOSTERS_N (sizeof(boosters) / sizeof(Booster *))

void setup()
{
	int i;

#ifdef CLI_ENABLED
	Serial.begin(CLI_SERIAL_SPEED);
	Serial.println("Initializing");
#endif

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

