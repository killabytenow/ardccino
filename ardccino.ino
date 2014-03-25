/*****************************************************************************
 * ardccino.ino
 *
 * Setup and main loop
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

#include <memorysaver.h>

#define __DECLARE_GLOBALS__ 1
#include "config.h"
#include "hwgui.h"

void setup(void)
{
	// install configured boosters
	BoosterMngr::set_boosters(boosters, BOOSTERS_N);

	// turn off all boosters
	off.enable();

	// enable interfaces
#ifdef CLI_ENABLED
	cli.init();
#endif
#ifdef HWGUI_ENABLED
	UIScreen::init(ui_hello);
#endif
}

void loop(void)
{
	BoosterMngr::refresh_current();
#ifdef CLI_ENABLED
	cli.input_read();
#endif
#ifdef HWGUI_ENABLED
	UIScreen::handle();
#ifdef SIMULATOR
	utft.gtk_refresh();
#endif
#endif
  
	delay(100);
}

