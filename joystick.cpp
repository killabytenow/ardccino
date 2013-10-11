/*****************************************************************************
 * joystick.h
 *
 * Joystick handler
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

#include "joystick.h"

#ifdef JOY_ENABLED

Joystick::Joystick(void)
{
	joyStatusNow = 0;
	joyStatusOld = 0;
}

void Joystick::read(void)
{
	int v;

	// reset joystick status
	joyStatusOld = joyStatusNow;
	joyStatusNow = 0;

	// update X axis
	v = analogRead(JOY_PIN_AXIS_X);
	if     (v < (JOY_RANGE_AXIS_X >> 2))                           joyStatusNow |= JOY_LEFT;
	else if(v > (JOY_RANGE_AXIS_X >> 1) + (JOY_RANGE_AXIS_X >> 2)) joyStatusNow |= JOY_RIGHT;

	// update Y axis
	v = analogRead(JOY_PIN_AXIS_Y);
	if     (v < (JOY_RANGE_AXIS_Y >> 2))                           joyStatusNow |= JOY_UP;
	else if(v > (JOY_RANGE_AXIS_Y >> 1) + (JOY_RANGE_AXIS_Y >> 2)) joyStatusNow |= JOY_DOWN;

	// update button
	if(analogRead(JOY_PIN_BUTTON) == 0) joyStatusNow |= JOY_BUTTON;
}

bool Joystick::move(uint8_t flag)
{
	return joyStatusNow & flag;
}

bool Joystick::pressed(uint8_t flag)
{
	return !(joyStatusOld & flag) && (joyStatusNow & flag);
}

void Joystick::print()
{
	if(move(JOY_UP))     Serial.print(" UP");
	if(move(JOY_DOWN))   Serial.print(" DOWN");
	if(move(JOY_LEFT))   Serial.print(" LEFT");
	if(move(JOY_RIGHT))  Serial.print(" RIGHT");
	if(move(JOY_BUTTON)) Serial.println(" [XX]"); else Serial.println(" [  ]");
}

#endif

