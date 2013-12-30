/*****************************************************************************
 * config.h
 *
 * Global configuration file
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <Arduino.h>
#include <UTFT.h>

#include "ansi.h"

#include "booster.h"
#include "off.h"
#include "pwm.h"
#include "dcc.h"

///////////////////////////////////////////////////////////////////////////////
// CONFIGURE BOOSTERS
//
// Configure names and pins assigned to each booster.
//
// NOTE:
//   - Pins 5 and 6 are paired on timer0
//   - Pins 9 and 10 are paired on timer1
//   - Pins 3 and 11 are paired on timer2
///////////////////////////////////////////////////////////////////////////////

#ifdef __DECLARE_GLOBALS__
Booster boosters[] = {
//	        name         pwm  dir  tmp  ocp  rst
//	        ------------ ---  ---  ---  ---  --- 
	Booster("booster#1",   3,   2,   4,   5,  14),
	Booster("booster#2",   9,   6,   7,   8,  14),
	Booster("booster#3",  10,  11,  12,  13,  14),
//	Booster("service",    xx,  xx,  xx,  xx,  xx),
};
#define BOOSTERS_N (sizeof(boosters) / sizeof(Booster))
#else
extern Booster boosters;
#endif

///////////////////////////////////////////////////////////////////////////////
// CHOOSE YOUR SERVICE BOOSTER
//
// In the following define you choose which booster is prepared for powering a
// service track.
//
// This booster will be independent of the other boosters (it will have its own
// DCC signal) and its current should be limited to 250 mA.
//
// If you don't want to have a service track booster then comment the following
// define or set it to -1.
///////////////////////////////////////////////////////////////////////////////

#define SERVICE_TRACK -1

///////////////////////////////////////////////////////////////////////////////
// CREATE BOOSTER MANAGERS
//
// Following code is usually OK (you need these three booster managers) and does
// not need to be edited.
///////////////////////////////////////////////////////////////////////////////

#ifdef __DECLARE_GLOBALS__
OffMngr off = OffMngr(boosters, BOOSTERS_N);
PwmMngr pwm = PwmMngr(boosters, BOOSTERS_N);
DccMngr dcc = DccMngr(boosters, BOOSTERS_N, -1);
//DccMngr dcc(boosters, 2, 1);
#else
extern OffMngr off;
extern PwmMngr pwm;
extern DccMngr dcc;
#endif

///////////////////////////////////////////////////////////////////////////////
// SERIAL CLI INTERFACE
//
// Serial CLI interface parameters
//
//   CLI_ENABLED
//     If defined booster is compiled with serial CLI support (default YES)
//
//   CLI_SERIAL_SPEED
//     Serial port bits/second or bauds. Default standard value is 9600, but
//     you can use the standard values 75, 110, 300, 1200, 2400, 4800, 9600,
//     19200, 38400, 57600 and 115200 bit.
//     The best, if you can, is to use 115200.
//     NOTE: If you use the 'make monitor' feature remember that you need to
//           sync this parameter with the MONITOR_BAUDRATE makefile environment
//           variable.
//
//   CLI
//

#define CLI_ENABLED 1

#ifdef CLI_ENABLED
#include "cli.h"
#define CLI_SERIAL_SPEED  115200
#define CLI_PROMPT        ANSI_SGR_RESET ANSI_SGR_BOLD "ardccino" ANSI_SGR_BOLD_OFF ">"
#ifdef __DECLARE_GLOBALS__
Cli cli = Cli();
#else
extern Cli cli;
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// CONFIGURE YOUR TFT HARDWARE
//
// Following code is usually OK (you need these three booster managers) and does
// not need to be edited.
//

//#define ENABLE_SCREEN 1

#ifdef ENABLE_SCREEN
#ifdef __DECLARE_GLOBALS__

// Elecfreaks TFT01-2.2SP 2.2 SPI 240 x 320 TFT LCD Module
#define TFT_SCK       44
#define TFT_SDI_MOSI  46
#define TFT_DC        48
#define TFT_RESET     50
#define TFT_CS        52
UTFT tft = UTFT(TFT01_22SP, TFT_SDI_MOSI, TFT_SCK, TFT_CS, TFT_RESET, TFT_DC);

#else
extern UTFT tft;
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// CONFIGURE JOYSTICK
//
// Joystick configuration.
//

//#define JOY_ENABLED 1

#ifdef JOY_ENABLED
#define JOY_PIN_AXIS_X    A0
#define JOY_PIN_AXIS_Y    A1
#define JOY_PIN_BUTTON    A2
#define JOY_RANGE_AXIS_X  1023
#define JOY_RANGE_AXIS_Y  1023
#endif

///////////////////////////////////////////////////////////////////////////////
// CONFIGURE HWGUI
//
// Hardware GUI configuration
//

//#define HWGUI_ENABLE 1

#ifndef JOY_ENABLED
#warning "Cannot enable hardware gui without an input method (enable joystick at least)"
#undef HWGUI_ENABLE
#endif

#ifndef ENABLE_SCREEN
#warning "Cannot enable hardware gui without a screen or video output"
#undef HWGUI_ENABLE
#endif

///////////////////////////////////////////////////////////////////////////////
// DON'T TOUCH THE FOLLOWING CODE
//
//   ... it's ok and works, really
//

#ifndef SERVICE_TRACK
#define SERVICE_TRACK -1
#endif

#if (!defined(HWGUI_ENABLE) && !defined(CLI_ENABLED))
#error "Is stupid to build a controller without control input methods - enable serial CLI at least"
#endif

#endif
