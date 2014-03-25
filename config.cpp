/*****************************************************************************
 * config.c
 *
 * Global configuration file -- this file only emits warnings and errors
 * depending on set configuration
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

///////////////////////////////////////////////////////////////////////////////
// EMIT WARNINGS AND ERRORS
///////////////////////////////////////////////////////////////////////////////

#ifdef JOY_ENABLED_WARNING
#warning "Cannot enable hardware gui without an input method (enable joystick at least)"
#endif

#ifdef ENABLE_SCREEN_WARNING
#warning "Cannot enable hardware gui without a screen or video output"
#endif

#if (!defined(HWGUI_ENABLED) && !defined(CLI_ENABLED))
#error "Is stupid to build a controller without control input methods - enable serial CLI at least"
#endif

///////////////////////////////////////////////////////////////////////////////
// DECLARE GLOBALS
///////////////////////////////////////////////////////////////////////////////

OffMngr off = OffMngr();
PwmMngr pwm = PwmMngr();
DccMngr dcc = DccMngr(SERVICE_TRACK);

