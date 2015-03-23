/*****************************************************************************
 * booster_mngr.cpp
 *
 * Generic booster manager.
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
#include "booster_mngr.h"

BoosterMngr *BoosterMngr::current = NULL;
Booster     *BoosterMngr::boosters = NULL;
int          BoosterMngr::nboosters = 0;

BoosterMngr *BoosterMngr::enable(void)
{
	if(BoosterMngr::current == this)
		return this;
	if(BoosterMngr::current)
		BoosterMngr::current->fini();
	BoosterMngr::current = this;
	init();
	return this;
}

int BoosterMngr::enabled(void)
{
	return this == BoosterMngr::current;
}

void BoosterMngr::refresh_current(void)
{
	BoosterMngr::current->refresh();
}

Booster *BoosterMngr::booster(uint8_t booster)
{
	if(booster > BoosterMngr::nboosters)
		cli.fatal("Booster #%d out of bounds", booster);
	return BoosterMngr::boosters + booster;
}

void BoosterMngr::set_boosters(Booster *ba, int n)
{
	BoosterMngr::boosters = ba;
	BoosterMngr::nboosters = n;
}

void BoosterMngr::on_all(void)
{
	for(int b = 0; b < BoosterMngr::nboosters; b++)
		this->on(b);
}

void BoosterMngr::off_all(void)
{
	for(int b = 0; b < BoosterMngr::nboosters; b++)
		this->off(b);
}

