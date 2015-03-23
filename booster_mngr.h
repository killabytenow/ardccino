/*****************************************************************************
 * booster_mngr.h
 *
 * Generic booster manager.
 *
 * ---------------------------------------------------------------------------
 * ardccino - Arduino dual PWM/DCC controller
 *   (C) 2013-2015 Gerardo García Peña <killabytenow@gmail.com>
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

#ifndef __BOOSTER_MNGR_H__
#define __BOOSTER_MNGR_H__

#include "booster.h"

class BoosterMngr {
public:
	static BoosterMngr *current;
	static Booster  *boosters;
	static int       nboosters;

	static void refresh_current(void);

	virtual void init(void) = 0;
	virtual void fini(void) = 0;
	virtual void refresh(void) = 0;

	virtual void on(uint8_t booster) = 0;
	virtual void off(uint8_t booster) = 0;

	void on_all();
	void off_all();

	BoosterMngr *enable(void);
	int enabled(void);

	static void set_boosters(Booster *ba, int n);
	static Booster *booster(uint8_t booster);
};

#endif
