/*****************************************************************************
 * booster_mngr.cpp
 *
 * Generic booster manager.
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

#include "config.h"
#include "booster_mngr.h"

BoosterMngr *BoosterMngr::current = NULL;

BoosterMngr::BoosterMngr(Booster *b, uint8_t n)
	: _boosters(b), _nboosters(n)
{
	if(!BoosterMngr::current)
		BoosterMngr::current = this;
	BoosterMngr::current->init();
}

BoosterMngr *BoosterMngr::enable(void)
{
	if(BoosterMngr::current)
		BoosterMngr::current->fini();
	BoosterMngr::current = this;
	this->init();
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
	if(current->booster < 0 || booster > current->_nboosters)
		cli.fatal("Booster #%d out of bounds", booster);
	return current->_boosters + booster;
}

uint8_t BoosterMngr::nboosters(void)
{
	return current->_boosters ? current->_nboosters : 0;
}

