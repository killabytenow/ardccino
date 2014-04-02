/*****************************************************************************
 * dcc.cpp
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

#include "config.h"
#include "interrupts.h"
#include "dcc.h"

#define DCC_CTC_ZERO    232
#define DCC_CTC_ONE     116

///////////////////////////////////////////////////////////////////////////////
// SIGNAL GENERATOR SUBROUTINES, ISR's AND METHOD
///////////////////////////////////////////////////////////////////////////////


#ifndef SIMULATOR
static byte dcc_msg_idle[3] = { 0xff, 0x00, 0xff };

ISR(TIMER1_COMPA_vect)
{
	for(int i = 0; i < BoosterMngr::nboosters; i++)
		if(i != dcc.service_booster)
			digitalWrite(BoosterMngr::boosters[i].dirSignalPin, dcc.operations.dccZero);
	dcc.isr(&dcc.operations, &OCR1A);

	// Capture the current timer value (TCTNx) for debugging purposes. It
	// always should be below 116 (a DCC one). Elsewhere the DCC generator
	// will produce corrupt signals :P
	unsigned int lat = TCNT1;
	if(lat >= DCC_CTC_ONE)
		dcc.operations.dcc_excesive_lat = lat;
}

ISR(TIMER3_COMPA_vect)
{
	if(dcc.service_booster < 0)
		return;
	digitalWrite(BoosterMngr::boosters[dcc.service_booster].dirSignalPin, dcc.service.dccZero);
	dcc.isr(&dcc.service, &OCR3A);

	// Capture the current timer value (TCTNx) for debugging purposes. It
	// always should be below 116 (a DCC one). Elsewhere the DCC generator
	// will produce corrupt signals :P
	unsigned int lat = TCNT3;
	if(lat >= DCC_CTC_ONE)
		dcc.service.dcc_excesive_lat = lat;
}

//#define OCRXX *OCR1x
#define OCRXX OCR1A
void DccMngr::isr(struct dcc_state *ds, volatile uint16_t *OCR1x)
{
	struct dcc_buffer_struct *cmsg;

	/* invert signal */
	if((ds->dccZero = !ds->dccZero))
		return;

	if(!ds->msg) {
		// we are still in preamble
		OCRXX = DCC_CTC_ONE;
		if(--ds->dccCurrentBit > 0)
			return;

		// select next msg
		ds->dcc_last_msg_id = (ds->dcc_last_msg_id + 1) & DCC_BUFFER_POOL_MASK;
		cmsg = ds->pool + ds->dcc_last_msg_id;
		if(cmsg->reps == 0) 
			cmsg->reps = -1;
		if(cmsg->reps > 0) {
			ds->msg         = cmsg->msg;
			ds->msg_pending = cmsg->len;
			cmsg->reps--;
		} else {
			ds->msg = dcc_msg_idle;
			ds->msg_pending = sizeof(dcc_msg_idle);
		}
		ds->dccCurrentBit = 0;
	} else {
		if(ds->dccCurrentBit) {
			// send data bit
			OCRXX = (*ds->msg & ds->dccCurrentBit) ? DCC_CTC_ONE : DCC_CTC_ZERO;
			ds->dccCurrentBit >>= 1;
			if(!ds->dccCurrentBit) {
				ds->msg++;
				ds->msg_pending--;
			}
		} else {
			// separator bit
			ds->dccCurrentBit = 0x80;
			if(ds->msg_pending <= 0) {
				ds->msg = NULL;
				ds->dccCurrentBit = 16;
				OCRXX = DCC_CTC_ONE; // last bit
			} else {
				OCRXX = DCC_CTC_ZERO; // more data will come
			}
		}
	}
}
#endif

DccMngr::DccMngr() : BoosterMngr()
{
	service_booster = -1;
}

DccMngr::DccMngr(int8_t service_booster)
	: BoosterMngr(), service_booster(service_booster)
{
	if(service_booster >= BoosterMngr::nboosters)
		cli.fatal("service_booster id is above nboosters.");
}

void DccMngr::init(void)
{
	disable_interrupts();

#ifndef SIMULATOR
	// CONFIGURE TIMER 1
	TCCR1A = (TCCR1A & 0b00001100)
	       | 0b00000000     // CTC  mode (!WGM11, !WGM10)
	       | 0b00000000     // OC1A disconnected
	       | 0b00000000;    // OC1B disconnected
	TCCR1B = (TCCR1B & 0b11100000)
	       | 0x2            // clock prescaler 0x02 (16e6/8 = 2 Mhz)
	       | 0b00001000;    // Normal mode (!WGM13, WGM12)
	OCR1B = 0;         // set compare registers
	OCR1A = DCC_CTC_ZERO;
	TIMSK1 = (TIMSK1 & 0b11011000)
	       | (1 << OCIE1A); // output cmp A match interrupt enable
		 
	// DISABLE TIMER 2
	TCCR2A = TCCR2A & 0b00001100;
	TCCR2B = TCCR2B & 0b11110000;
	OCR2A = OCR2B = 0;
	TIMSK2 &= 0b11111000;

	if(service_booster >= 0) {
		// CONFIGURE TIMER 3
		TCCR3A = (TCCR3A & 0b00001100)
		       | 0b00000000     // CTC  mode (!WGM11, !WGM10)
		       | 0b00000000     // OC3A disconnected
		       | 0b00000000;    // OC3B disconnected
		TCCR3B = (TCCR3B & 0b11100000)
		       | 0x2            // clock prescaler 0x02 (16e6/8 = 2 Mhz)
		       | 0b00001000;    // Normal mode (!WGM13, WGM12)
		OCR3B = 0xffff;         // set compare registers
		OCR3A = DCC_CTC_ZERO;
		TIMSK3 = (TIMSK3 & 0b11011000)
		       | (1 << OCIE3A); // output cmp A match interrupt enable
	}
#endif

	// SET PWM OUTPUTS TO 1
	for(int b = 0; b < BoosterMngr::nboosters; b++)
		digitalWrite(BoosterMngr::boosters[b].pwmSignalPin, BoosterMngr::boosters[b].enabled);

	//-------------------------------------------
	// RESET DCC STATUS
	memset(&operations, 0, sizeof(operations));
	memset(&service,    0, sizeof(service));
	operations.dccCurrentBit = service.dccCurrentBit = 16;

	enable_interrupts();
}

void DccMngr::fini(void)
{
	disable_interrupts();

	// disconnect all OC1A:B/OC2A:B pins and go to normal operations mode
#ifndef SIMULATOR
	TCCR1A = TCCR1A & 0b00001100;    TCCR2A = TCCR2A & 0b00001100;
	TCCR1B = TCCR1B & 0b11100000;    TCCR2B = TCCR2B & 0b11110000;
	OCR1A = OCR1B = 0;               OCR2A = OCR2B = 0;
	TIMSK1 &= 0b11011000;            TIMSK2 &= 0b11111000;
#endif

	enable_interrupts();
}

uint16_t DccMngr::get_address(uint16_t address, uint8_t addr_type)
{
	if(addr_type == DCC_ADDR_TYPE_NONE)
		addr_type = default_addr_type;
	
again:
	switch(addr_type) {
	default:
	case DCC_ADDR_TYPE_7BIT:
		if(address > 0x7f || !address) {
			cli.error("Invalid 7bit address (%d).", address);
			return DCC_DECO_ADDR_BAD;
		}
		address |= DCC_DECO_ADDR_7BIT;
		break;

	case DCC_ADDR_TYPE_14BIT:
		if(address > 0x27ff) {
			cli.error("Invalid 14-bit address (%d)", address);
			return DCC_DECO_ADDR_BAD;
		}
		address |= DCC_DECO_ADDR_14BIT;
		break;

	case DCC_ADDR_TYPE_AUTO:
		addr_type = address < 0x80 && address != 0
				? DCC_ADDR_TYPE_7BIT
				: DCC_ADDR_TYPE_14BIT;
		goto again;
	}

	return address;
}

struct dcc_buffer_struct *DccMngr::slot_get(bool service_track, uint16_t address)
{
	int i;
	struct dcc_buffer_struct *slot, *pool;

	pool = service_track ? service.pool : operations.pool;

	// invalidate pending instructions to this address
	for(i = 0, slot = pool; i < DCC_BUFFER_POOL_SIZE; i++, slot++)
		if(slot->reps > 0 && address == slot->address) {
			slot->reps = 0;
			slot->address = 0xffff;
		}

	// find a free slot
	for(i = 0, slot = pool; i < DCC_BUFFER_POOL_SIZE; i++, slot++)
		if(slot->reps < 0)
			break;
	if(i >= DCC_BUFFER_POOL_SIZE) {
		cli.error("No free slots.");
		return NULL;
	}

	// fill slot (but dont commit)
	slot->address = address;
	if(address & DCC_DECO_ADDR_14BIT) {
		slot->msg[0] = address & 0x00ff;
		slot->msg[1] = address >> 8;
		slot->len = 2;
	} else {
		slot->msg[0] = address & 0x00ff;
		slot->len = 1;
	}
	return slot;
}

bool DccMngr::slot_commit(struct dcc_buffer_struct *slot)
{
	byte chksum, *msg;

	if(slot->len < 2 || slot->len >= DCC_MSG_MAX) {
		cli.error("Bad message size");
		return false;
	}

	// calculate checksum
	chksum = 0;
	msg = slot->msg;
	for(uint8_t i = 0; i < slot->len; i++)
		chksum ^= *msg++;
	*msg = chksum;
	slot->len++;

	// commit message
	slot->reps = 5;

	return true;
}

bool DccMngr::set_speed(bool service_track, uint16_t address, uint16_t speed)
{
	struct dcc_buffer_struct *slot;
	byte *msg;

	if(!(slot = slot_get(service_track, address)))
		return false;
	msg = slot->msg + slot->len;

	if((speed & DCC_DECO_SPEED_MASK) == DCC_DECO_SPEED_7BIT) {
		msg[0] = 0x3f; // 128 speed control command
		msg[1] = speed & ~DCC_DECO_SPEED_MASK;
		slot->len += 2;
	} else {
		msg[0] = 0x40 | (speed & 0x3f);
		slot->len++;
	}
	return slot_commit(slot);
}

void DccMngr::refresh(void)
{
#if 0
	static int lolol = 0;
	if(lolol++ >= 30) {
		lolol = 0;

		cli.debug("DCC msg=%p msg_pending=%d currbit=%d l_id=%d",
				operations.msg,
				operations.msg_pending,
				operations.dccCurrentBit,
				operations.dcc_last_msg_id);
		for(int i = 0; i < DCC_BUFFER_POOL_SIZE; i++) {
			cli.debug("  pool[%d] = %x %d %d",
					i,
					operations.pool[i].address,
					operations.pool[i].len,
					operations.pool[i].reps);
		}

#if 0
		static int dir = 20;
		dir = 0 - dir;
		if(!set_speed(false, DCC_DECO_ADDR_GET7(3), DCC_DECO_SPEED_GET5(5)))
			cli.debug("cannot send speed");
#endif
	}
#endif

	// warn about excesive latencies
	if(operations.dcc_excesive_lat) {
		cli.error("operations.dcc_excesive_lat = %d", operations.dcc_excesive_lat);
		operations.dcc_excesive_lat = 0;
	}
	if(service.dcc_excesive_lat) {
		cli.error("service.dcc_excesive_lat = %d", service.dcc_excesive_lat);
		service.dcc_excesive_lat = 0;
	}
}

