/*****************************************************************************
 * booster.h
 *
 * Booster definition and configuration object.
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

#ifndef __BOOSTER_H__
#define __BOOSTER_H__

#include "config.h"

class Booster {
public:
	// GENERIC INFO
	char    *name;
	uint8_t  pwmSignalPin;
	uint8_t  dirSignalPin;
	uint8_t  tmpAlarmPin;  // temperature alarm
	uint8_t  ocpAlarmPin;  // over current protection alarm
	uint8_t  rstSignalPin; // reset pin
	bool     enabled;

	// ANALOG CONTROL (PWM, analog, etc)
	int      trgt_power;
	int      curr_power;
	int      curr_accel;
	int      inc_accel;
	int      min_power;
	int      max_power;
	int      max_accel;
	bool     inertial;

	// METHODS
	Booster(char   *name,
		uint8_t pwmSignalPin, 
		uint8_t dirSignalPin,
		uint8_t tmpAlarmPin,
		uint8_t ocpAlarmPin,
		uint8_t rstSignalPin);
	void reset(void);
	void on(void);
	void off(void);
	void set_inc_accel(int a);
	void set_min_power(int a);
	void set_max_power(int a);
	void set_max_accel(int a);
	void set_mode_inertial(void);
	void set_mode_direct(void);
};

#endif
