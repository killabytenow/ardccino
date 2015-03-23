/*****************************************************************************
 * off.cpp
 *
 * The null energy-saving booster manager.
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
#include "off.h"

void OffMngr::init(void)
{
}

void OffMngr::fini(void)
{
}

void OffMngr::refresh(void)
{
}

static void configure_booster(uint8_t b)
{
	digitalWrite(BoosterMngr::boosters[b].pwmSignalPin, LOW);
	digitalWrite(BoosterMngr::boosters[b].dirSignalPin, LOW);
}

void OffMngr::on(uint8_t b)
{
	BoosterMngr::boosters[b].enabled = true;
	configure_booster(b);
}

void OffMngr::off(uint8_t b)
{
	BoosterMngr::boosters[b].enabled = false;
	configure_booster(b);
}

