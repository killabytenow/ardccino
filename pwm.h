/*****************************************************************************
 * pwm.h
 *
 * PWM booster manager.
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

#ifndef __PWM_H__
#define __PWM_H__

#include "booster.h"
#include "booster_mngr.h"

class PwmMngr : public BoosterMngr {
private:
	void booster_refresh(Booster *b);
	void accelerate(Booster *b, int v);

public:
	void init(void);
	void fini(void);
	void refresh(void);

	void accelerate(int b, int v);
	void speed(int b, int s);
	void stop(int b);
	void switch_dir(int b);
};

#endif
