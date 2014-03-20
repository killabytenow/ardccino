/*****************************************************************************
 * dcc.h
 *
 * Digital DCC booster manager.
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

#ifndef __DCC_H__
#define __DCC_H__

#include "config.h"
#include "booster.h"
#include "booster_mngr.h"

#define DCC_MSG_MAX           5
#define DCC_BUFFER_POOL_BITS  3
#define DCC_BUFFER_POOL_MASK  ((char) ~(0xff << DCC_BUFFER_POOL_BITS))
#define DCC_BUFFER_POOL_SIZE  (1 << DCC_BUFFER_POOL_BITS)

struct dcc_buffer_struct {
  byte      msg[DCC_MSG_MAX];
  uint16_t  address;
  uint8_t   len;
  uint8_t   reps;
//uint16_t  command;
};

class DccMngr : public BoosterMngr {
private:
	int8_t                    service_booster;
	void isr(struct dcc_buffer_struct *pool);

public:
	struct dcc_buffer_struct ope_buffer_pool[DCC_BUFFER_POOL_SIZE];
	struct dcc_buffer_struct srv_buffer_pool[DCC_BUFFER_POOL_SIZE];

	DccMngr(Booster *b, uint8_t n);
	DccMngr(Booster *b, uint8_t n, int8_t service);
	void init(void);
	void fini(void);
	void refresh(void);

	void isr_operations(void);
	void isr_service(void);

	struct dcc_buffer_struct *send_msg(bool service, byte *msg, uint8_t len);
};

#endif
