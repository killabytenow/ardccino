/*****************************************************************************
 * dcc.cpp
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
#if SERVICE_TRACK >= 0
		if(i != SERVICE_TRACK)
#endif
			digitalWrite(
				BoosterMngr::boosters[i].dirSignalPin,
				dcc.operations.dccZero && BoosterMngr::boosters[i].enabled);
	dcc.isr(&dcc.operations, &OCR1A);

	// Capture the current timer value (TCTNx) for debugging purposes. It
	// always should be below 116 (a DCC one). Elsewhere the DCC generator
	// will produce corrupt signals :P
	unsigned int lat = TCNT1;
	if(lat >= DCC_CTC_ONE)
		dcc.operations.dcc_excesive_lat = lat;
}

#if SERVICE_TRACK >= 0
ISR(TIMER3_COMPA_vect)
{
	digitalWrite(
		BoosterMngr::boosters[SERVICE_TRACK].dirSignalPin,
		dcc.service.dccZero && BoosterMngr::boosters[SERVICE_TRACK].enabled);
	dcc.isr(&dcc.service, &OCR3A);

	// Capture the current timer value (TCTNx) for debugging purposes. It
	// always should be below 116 (a DCC one). Elsewhere the DCC generator
	// will produce corrupt signals :P
	unsigned int lat = TCNT3;
	if(lat >= DCC_CTC_ONE)
		dcc.service.dcc_excesive_lat = lat;
}
#endif

void DccMngr::isr(struct dcc_state *ds, volatile uint16_t *OCRnx)
{
	struct dcc_buffer_struct *cmsg;

	/* invert signal */
	if((ds->dccZero = !ds->dccZero))
		return;

	if(!ds->msg) {
		// we are still in preamble
		*OCRnx = DCC_CTC_ONE;
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
			*OCRnx = (*ds->msg & ds->dccCurrentBit) ? DCC_CTC_ONE : DCC_CTC_ZERO;
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
				*OCRnx = DCC_CTC_ONE; // last bit
			} else {
				*OCRnx = DCC_CTC_ZERO; // more data will come
			}
		}
	}
}
#endif

DccMngr::DccMngr() : BoosterMngr()
{
}

static void booster_activate(uint8_t b)
{
	digitalWrite(
		BoosterMngr::boosters[b].pwmSignalPin,
		BoosterMngr::boosters[b].enabled);
}

static void booster_deactivate(uint8_t b)
{
	digitalWrite(BoosterMngr::boosters[b].dirSignalPin, 0);
	digitalWrite(BoosterMngr::boosters[b].pwmSignalPin, 0);
}

void DccMngr::init(void)
{
	if(SERVICE_TRACK >= BoosterMngr::nboosters)
		cli.fatal(P("SERVICE_TRACK id is above nboosters."));

	disable_interrupts();

#ifndef SIMULATOR
	// CONFIGURE TIMER 1/A
	TCCR1A = (TCCR1A & 0b00001100)
	       | 0b00000000     // CTC  mode (!WGM11, !WGM10)
	       | 0b00000000     // OC1A disconnected
	       | 0b00000000;    // OC1B disconnected
	TCCR1B = (TCCR1B & 0b11100000)
	       | 0x2            // clock prescaler 0x02 (16e6/8 = 2 Mhz)
	       | 0b00001000;    // Normal mode (!WGM13, WGM12)
	OCR1A = DCC_CTC_ONE;
	TIMSK1 = (TIMSK1 & 0b11011000)
	       | (1 << OCIE1A); // output cmp A match interrupt enable
		 
#if SERVICE_TRACK >= 0
	// CONFIGURE TIMER 3
	TCCR3A = (TCCR3A & 0b00001100)
	       | 0b00000000     // CTC  mode (!WGM11, !WGM10)
	       | 0b00000000     // OC3A disconnected
	       | 0b00000000;    // OC3B disconnected
	TCCR3B = (TCCR3B & 0b11100000)
	       | 0x2            // clock prescaler 0x02 (16e6/8 = 2 Mhz)
	       | 0b00001000;    // Normal mode (!WGM13, WGM12)
	OCR3B = 0xffff;         // set compare registers
	OCR3A = DCC_CTC_ONE;
	TIMSK3 = (TIMSK3 & 0b11011000)
	       | (1 << OCIE3A); // output cmp A match interrupt enable
#endif
#endif

	// SET PWM OUTPUTS TO 1
	for(int b = 0; b < BoosterMngr::nboosters; b++)
		booster_activate(b);

	//-------------------------------------------
	// RESET DCC STATUS
	memset(&operations, 0, sizeof(operations));
	memset(&service,    0, sizeof(service));
	operations.dccCurrentBit = service.dccCurrentBit = 16;

	enable_interrupts();
}

void DccMngr::fini(void)
{
	// disconnect all timers
	disable_interrupts();
#ifndef SIMULATOR
	// timer 1
	TCCR1A = TCCR1A & 0b00001100;
	TCCR1B = TCCR1B & 0b11100000;
	OCR1A = OCR1B = 0;
	TIMSK1 &= 0b11011000;

#if SERVICE_TRACK >= 0
	// timer 3
	TCCR3A = TCCR3A & 0b00001100;
	TCCR3B = TCCR3B & 0b11100000;
	OCR3A = OCR3B = 0;           
	TIMSK3 &= 0b11011000;        
#endif
#endif
	enable_interrupts();

	for(int b = 0; b < BoosterMngr::nboosters; b++)
		booster_deactivate(b);
}

void DccMngr::on(uint8_t b)
{
	BoosterMngr::boosters[b].enabled = true;
	booster_activate(b);
}

void DccMngr::off(uint8_t b)
{
	BoosterMngr::boosters[b].enabled = false;
	booster_deactivate(b);
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
			cli.error(P("Invalid 7bit address (%d)."), address);
			return DCC_DECO_ADDR_BAD;
		}
		address |= DCC_DECO_ADDR_7BIT;
		break;

	case DCC_ADDR_TYPE_14BIT:
		if(address > 0x27ff) {
			cli.error(P("Invalid 14-bit address (%d)"), address);
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
		cli.error(P("No free slots."));
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
		cli.error(P("Bad message size"));
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

bool DccMngr::set_light(bool service_track, uint16_t address, bool on)
{
	struct dcc_buffer_struct *slot;
	byte *msg;

	if(!(slot = slot_get(service_track, address)))
		return false;
	msg = slot->msg + slot->len;

	msg[0] = on ? 0b10010000 : 0b10000000;
	slot->len++;

	return slot_commit(slot);
}

bool DccMngr::reset(bool service_track)
{
	struct dcc_buffer_struct *slot;

	if(!(slot = slot_get(service_track, 0x00)))
		return false;
	*(slot->msg + slot->len++) = 0;

	return slot_commit(slot);
}

void DccMngr::status(void)
{
	cli.info(P("DCC msg=%p msg_pending=%d currbit=%d l_id=%d"),
			operations.msg,
			operations.msg_pending,
			operations.dccCurrentBit,
			operations.dcc_last_msg_id);
	for(int i = 0; i < DCC_BUFFER_POOL_SIZE; i++) {
		cli.info(P("  pool[%d] = %x %d %d"),
				i,
				operations.pool[i].address,
				operations.pool[i].len,
				operations.pool[i].reps);
	}
}

void DccMngr::refresh(void)
{
	// warn about excesive latencies
	if(operations.dcc_excesive_lat) {
		cli.error(P("operations.dcc_excesive_lat = %d"), operations.dcc_excesive_lat);
		operations.dcc_excesive_lat = 0;
	}
	if(service.dcc_excesive_lat) {
		cli.error(P("service.dcc_excesive_lat = %d"), service.dcc_excesive_lat);
		service.dcc_excesive_lat = 0;
	}
}

