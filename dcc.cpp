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
//#include "USBAPI.h"
#include "dcc.h"

#define DCC_CTC_ZERO    232
#define DCC_CTC_ONE     116

///////////////////////////////////////////////////////////////////////////////
// ISR METHODS AND INTERRUPTION HANDLERS
// (disabled automatically in simulator)
///////////////////////////////////////////////////////////////////////////////

unsigned dcc_excesive_lat;    // Debugging flag: Excesive latency

#ifndef SIMULATOR
static byte dcc_msg_idle[4] = { 0xff, 0x00, 0xff, 0x00 };

int dcc_last_msg_id;          // Index of the last message sent
char msg_pending;             // Bytes pending to be sent
byte *msg = NULL;             // Pointer into the current msg

ISR(TIMER1_COMPA_vect)
{
	dcc.isr_operations();

	// Capture the current timer value (TCTNx) for debugging purposes. It always
	// should be below 116 (a DCC one). Elsewhere the DCC generator will produce
	// corrupt signals :P
	//unsigned int 
	unsigned int lat = TCNT1;
	if(lat >= DCC_CTC_ONE)
		dcc_excesive_lat = lat;
}

ISR(TIMER1_COMPB_vect)
{
	dcc.isr_service();

	// Capture the current timer value (TCTNx) for debugging purposes. It always
	// should be below 116 (a DCC one). Elsewhere the DCC generator will produce
	// corrupt signals :P
	//unsigned int 
	unsigned int lat = TCNT1;
	if(lat >= DCC_CTC_ONE)
		dcc_excesive_lat = lat;
}

void DccMngr::isr_operations(void)
{
	isr(ope_buffer_pool);
}

void DccMngr::isr_service(void)
{
	isr(ope_buffer_pool);
}

void DccMngr::isr(struct dcc_buffer_struct *pool)
{
	static char dccZero = 0;
	static unsigned char dccCurrentBit;

	struct dcc_buffer_struct *cmsg;

	/* invert signal */
	for(int i = 0; i < BoosterMngr::nboosters; i++)
		digitalWrite(BoosterMngr::boosters[i].pwmSignalPin, BoosterMngr::boosters[i].enabled);
	if((dccZero = !dccZero))
		return;

	if(!msg) {
		// we are still in preamble
		OCR1A = DCC_CTC_ONE;
		if(--dccCurrentBit > 0)
			return;

		// select next msg
		dcc_last_msg_id = (dcc_last_msg_id + 1) & DCC_BUFFER_POOL_MASK;
		cmsg = pool + dcc_last_msg_id;
		if(cmsg->reps == 0) 
			cmsg->reps = -1;
		if(cmsg->reps > 0) {
			msg         = cmsg->msg;
			msg_pending = cmsg->len;
			cmsg->reps--;
		} else {
			msg         = dcc_msg_idle;
			dcc_msg_idle[3] = dcc_msg_idle[0] ^ dcc_msg_idle[1]  ^ dcc_msg_idle[2];
			msg_pending = sizeof(dcc_msg_idle);
		}
		dccCurrentBit = 0;
	} else {
		if(dccCurrentBit) {
			// send data bit
			OCR1A = (*msg & dccCurrentBit) ? DCC_CTC_ONE : DCC_CTC_ZERO;
			dccCurrentBit >>= 1;
			if(!dccCurrentBit) {
				msg++;
				msg_pending--;
			}
		} else {
			// separator bit
			dccCurrentBit = 0x80;
			if(msg_pending <= 0) {
				msg = NULL;
				dccCurrentBit = 16;
				OCR1A = DCC_CTC_ONE; // last bit
			} else {
				OCR1A = DCC_CTC_ZERO; // more data will come
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
	cli.debug("Initializing DCC.");

	disable_interrupts();

	// CONFIGURE TIMER 1
#ifndef SIMULATOR
	TCCR1A = (TCCR1A & 0b00001100)
	       | 0b00000000     // CTC  mode (!WGM11, !WGM10)
	       | 0b00000000     // OC1A disconnected
	       | 0b00000000;    // OC1B disconnected
	TCCR1B = (TCCR1B & 0b11100000)
	       | 0x2            // clock prescaler 0x02 (16e6/8 = 2 Mhz)
	       | 0b00001000;    // Normal mode (!WGM13, WGM12)
	OCR1B = 0;      // set compare registers to 0
	OCR1A = DCC_CTC_ZERO;
	TIMSK1 = (TIMSK1 & 0b11011000)
	       | (1 << OCIE1A); // output cmp A match interrupt enable
		 
	// DISABLE TIMER 2
	TCCR2A = TCCR2A & 0b00001100;
	TCCR2B = TCCR2B & 0b11110000;
	OCR2A = OCR2B = 0;
	TIMSK2 &= 0b11111000;
#endif

	// SET PWM OUTPUTS TO 1
	for(int b = 0; b < BoosterMngr::nboosters; b++)
		digitalWrite(BoosterMngr::boosters[b].pwmSignalPin, BoosterMngr::boosters[b].enabled);

	//-------------------------------------------
	// RESET DCC STATUS
	memset(ope_buffer_pool, 0, sizeof(ope_buffer_pool));
	memset(srv_buffer_pool, 0, sizeof(srv_buffer_pool));

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

unsigned int dir = 0;
void DccMngr::refresh(void)
{
	static unsigned int lolol = 0;

	// update only each 20 iterations
	if(lolol++ >= 20) {
		//byte msg[] = { 3, 0b10000000 };
		//msg[1] |= dir ? 0b10010000 : 0;
		byte msg[] = { 3, 0b01001000 };
		msg[1] |= dir ? 0b00100000 : 0;
		dir = !dir;
		lolol = 0;
		struct dcc_buffer_struct *b = send_msg(false, msg, sizeof(msg));
		if(b) {
			cli.debug("nmsg.len = %d", b->len);
			for(unsigned char i = 0; i < b->len; i++)
				cli.debug("    msg[%d] = %d", i, b->msg[i]);
		}
	}

	// warn about excesive latencies
	if(dcc_excesive_lat)
		cli.error("dcc_excesive_lat = %d", dcc_excesive_lat);
}

struct dcc_buffer_struct *DccMngr::send_msg(bool service, byte *msg, uint8_t len)
{
	unsigned int address;
	struct dcc_buffer_struct *selected_buffer;
	struct dcc_buffer_struct *buffer_pool =
		service ? srv_buffer_pool
			: ope_buffer_pool;
  
	if(len < 2 || len >= DCC_MSG_MAX)
		cli.fatal("Bad message size");
  
	// extract address
	address = msg[0]; // enough for 7-bit address, or broadcast (0x00) or idle packets (0xff)
	if(address & 0x80 && address != 0xff) {
		// yep! two-byte address
		if(len == 2)
			cli.fatal("Message too short (2 byte address in a 2 byte packet)");
		address = address << 8 | msg[1];
	}
  
	// extract command
	//command = *n & 0b11100000;
	//if(command == 0b01100000)
	//  command = 0b11000000;
  
	// search free buffer and kill other related buffers
	selected_buffer = NULL;
	for(int i = 0; i < DCC_BUFFER_POOL_SIZE; i++) {
		if(address == buffer_pool[i].address
		|| !buffer_pool[i].address) {
			buffer_pool[i].reps = 0;
			buffer_pool[i].address = 0xffff;
		}
		if(!selected_buffer && buffer_pool[i].reps < 0)
			selected_buffer = buffer_pool + i;
	}
	cli.debug("selected_buffer=%d", selected_buffer);
	if(!selected_buffer)
		return NULL;

	// copy msg to selected buffer
	byte x = 0;
	for(unsigned char i = 0; i < len; i++)
		x ^= selected_buffer->msg[i] = msg[i];
	selected_buffer->msg[len] = x;
	selected_buffer->len = len + 1;
	//selected_buffer->command = command;
	selected_buffer->address = address;
	selected_buffer->reps = 100;

	return selected_buffer;
}

