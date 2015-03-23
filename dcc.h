/*****************************************************************************
 * dcc.h
 *
 * Digital DCC booster manager.
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
  int8_t    reps;
};

struct dcc_state {
	// DCC MSG BUFFER
	struct dcc_buffer_struct  pool[DCC_BUFFER_POOL_SIZE];
	byte                     *msg;              // Pointer into the current msg
	int                       dcc_last_msg_id;  // Index of the last message sent
	char                      msg_pending;      // Bytes pending to be sent
	char                      dccZero;
	unsigned char             dccCurrentBit;
	unsigned                  dcc_excesive_lat; // Debugging flag: Excesive latency
};

struct dcc_deco {
	uint16_t	addr;
	#define DCC_DECO_ADDR_7BIT  	0x0000
	#define DCC_DECO_ADDR_14BIT 	0xc000
	#define DCC_DECO_ADDR_BAD   	0xffff
	#define DCC_DECO_ADDR_BROADCAST	(DCC_DECO_ADDR_7BIT | 0x0000)
	uint16_t	speed;
	#define DCC_DECO_SPEED_4BIT 0x0000
	#define DCC_DECO_SPEED_5BIT 0x0100
	#define DCC_DECO_SPEED_7BIT 0x0200
	#define DCC_DECO_SPEED_BAD  0xffff
	#define DCC_DECO_SPEED_MASK 0xff00
};

static inline uint16_t DCC_DECO_SPEED_GET7(int16_t s)
{
	uint16_t __x = s;
	if(s < -127 || s > 127)
		return DCC_DECO_SPEED_BAD;
	return ((__x & 0x80) ? ((~__x + 1) & 0x7f) | 0x80 : __x);
}
static inline uint16_t DCC_DECO_SPEED_GET5(int16_t s)
{
	uint16_t __x = s;
	if(s < -31 || s > 31)
		return DCC_DECO_SPEED_BAD;
	return ((__x & 0x80) ? ((~__x + 1) & 0x1f) | 0x20 : __x);
}
static inline uint16_t DCC_DECO_SPEED_GET4(int16_t s, bool l)
{
	uint16_t __x = s;
	if(s < -15 || s > 15)
		return DCC_DECO_SPEED_BAD;
	return ((__x & 0x80) ? ((~__x + 1) & 0x0f) | 0x20 : __x)
		| (l ? 0x10 : 0x00);
}

#define DCC_DECO_ADDR_GET7(x)           ((x) & 0x7f)
#define DCC_DECO_ADDR_GET14(x)          (((x) & 0xc000) | 0xc000)

class DccMngr : public BoosterMngr {
public:
	int8_t   service_booster;
	struct   dcc_state operations;
	struct   dcc_state service;
	int8_t   default_addr_type;
#define DCC_ADDR_TYPE_NONE	0
#define DCC_ADDR_TYPE_7BIT	1
#define DCC_ADDR_TYPE_14BIT	2
#define DCC_ADDR_TYPE_AUTO	3

	void isr(struct dcc_state *ds, volatile uint16_t *OCR1x);

	DccMngr();
	DccMngr(int8_t service);
	void init(void);
	void fini(void);
	void refresh(void);

	void on(uint8_t booster);
	void off(uint8_t booster);

	// low level dcc methods
	struct dcc_buffer_struct *slot_get(bool service_track, uint16_t address);
	bool slot_commit(struct dcc_buffer_struct *slot);

	// dcc methods
	uint16_t get_address(uint16_t address, uint8_t addr_type);
	bool set_speed(bool service_track, uint16_t address, uint16_t speed);
};

#endif
