/*****************************************************************************
 * booster.cpp
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

#include "Arduino.h"
#include "booster.h"

Booster::Booster(char   *name,
		 uint8_t pwmSignalPin, 
		 uint8_t dirSignalPin,
		 uint8_t tmpAlarmPin,
		 uint8_t ocpAlarmPin,
		 uint8_t rstSignalPin)
	: name(name),
	  pwmSignalPin(pwmSignalPin), dirSignalPin(dirSignalPin),
	  tmpAlarmPin(tmpAlarmPin),   ocpAlarmPin(ocpAlarmPin),
	  rstSignalPin(rstSignalPin)
{
	Serial.print("Configuring booster ["); Serial.print(name); Serial.println(")");
	trgt_power   = 0;
	curr_power   = 0;
	curr_accel   = 0;
	inc_accel    = 7;
	min_power    = 60;
	max_power    = 255;
	max_accel    = 40;
	inertial     = true;
	enabled      = true;

	pinMode(pwmSignalPin, OUTPUT); digitalWrite(pwmSignalPin, LOW);
	pinMode(dirSignalPin, OUTPUT); digitalWrite(dirSignalPin, LOW);
	pinMode(rstSignalPin, OUTPUT); digitalWrite(rstSignalPin, LOW);
	pinMode(tmpAlarmPin, INPUT);
	pinMode(ocpAlarmPin, INPUT);
}

void Booster::reset(void)
{
	// set power to 0
	trgt_power   = 0;
	curr_power   = 0;
	curr_accel = 0;

	// enable booster
	enabled      = true;

	// reset booster (with a 10 ms pulse)
	digitalWrite(rstSignalPin, HIGH);
	delay(10);
	digitalWrite(rstSignalPin, LOW);
}

void Booster::on(void)
{
	enabled = true;
}

void Booster::off(void)
{
	enabled = false;
}

void Booster::set_inc_accel(int a)
{
#warning "TODO"
}

void Booster::set_min_power(int a)
{
#warning "TODO"
}

void Booster::set_max_power(int a)
{
#warning "TODO"
}

void Booster::set_max_accel(int a)
{
#warning "TODO"
}

