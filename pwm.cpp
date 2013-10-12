/*****************************************************************************
 * pwm.cpp
 *
 * PWM booster manager.
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
#include "pwm.h"
#include "interrupts.h"

void PwmMngr::init(void)
{
	uint8_t timers;

	// discover which timers must be configured
	for(int b = 0; b < nboosters; b++) {
		switch(boosters[b].pwmSignalPin) {
		case 9:
		case 10:
			timers |= (1 << 1);
			break;
		case 3:
		case 11:
			timers |= (1 << 2);
			break;
		default:
			cli.fatal("Cannot figure how to configure output pin for PWM output");
		}
	}

	disable_interrupts();

	// CONFIGURE TIMERS
	if(timers & (1 << 1)) {
		// timer 1
		TCCR1A = (TCCR1A & 0b00001100)
		       | 0b00000001;  // Fast PWM 8-bit (!WGM11, WGM10)
		TCCR1B = (TCCR1B & 0b11100000)
		       | 0x4          // clock prescaler 0x04 (16e6/256 = 62.5 khz)
		       | 0b00001000;  // Fast PWM 8-bit (!WGM13, WGM12)
		OCR1A = OCR1B = 0;    // set compare registers to 0
		TIMSK1 &= 0b11011000; // disable all timer 1 interrupts
	}

	if(timers & (1 << 2)) {
		// timer 2
		TCCR2A = (TCCR2A & 0b00001100)
		       | 0b00000011;  // Fast PWM (WGM21, WGM20)
		TCCR2B = (TCCR2B & 0b11110000)
		       | 0x6          // clock prescaler 0x6 (16e6/256 = 62.5 khz)
		       | 0b00000000;  // Fast PWM (!WGM22)
		OCR2A = OCR2B = 0;    // set compare registers to 0
		TIMSK2 &= 0b11111000; // disable all timer 2 interrupts
	}

	// CONFIGURE OUTPUT PINS FOR DIRECT PWM OUTPUT
	for(int b = 0; b < nboosters; b++) {
		switch(boosters[b].pwmSignalPin) {
		case 9:  TCCR1A |= 0b10000000; break; // Clear OC1A on cmp match
		case 10: TCCR1A |= 0b00100000; break; // Clear OC1B on cmp match
		case 11: TCCR2A |= 0b10000000; break; // Clear OC2A on cmp match
		case 3:  TCCR2A |= 0b00100000; break; // Clear OC2B on cmp match
		default: cli.fatal("Cannot configure output pin for PWM output");
		}
	}

	//-------------------------------------------
	// RESET PWM STATUS
	for(int b = 0; b < nboosters; b++)
		boosters[b].reset();

	enable_interrupts();
}

void PwmMngr::fini(void)
{
	disable_interrupts();

	// disconnect all OC1A:B/OC2A:B pins and go to normal operations mode
	TCCR1A = TCCR1A & 0b00001100;    TCCR2A = TCCR2A & 0b00001100;
	TCCR1B = TCCR1B & 0b11100000;    TCCR2B = TCCR2B & 0b11110000;
	OCR1A = OCR1B = 0;               OCR2A = OCR2B = 0;
	TIMSK1 &= 0b11011000;            TIMSK2 &= 0b11111000;

	enable_interrupts();
}

void PwmMngr::booster_refresh(Booster *b)
{
	if(b->trgt_power == b->curr_power)
		return;

	if(b->inertial) {
		// INERTIAL MODE
		int d, trgt_power, acc_inc;

		// If target of different sign than current val then brake to zero...
		// elsewhere go directly to the target speed
		trgt_power = b->curr_power && (b->trgt_power ^ b->curr_power) & ((int) 0x8000)
				? 0
				: b->trgt_power;

		// calculate acceleration increment
		acc_inc = trgt_power > b->curr_power
				?  b->inc_accel
				: -b->inc_accel;
		if(abs(b->curr_accel) < b->max_accel)
			b->curr_accel += acc_inc;

		// calculate final speed
		if(abs(trgt_power - b->curr_power) < abs(b->curr_accel)) {
			d = (trgt_power - b->curr_power) >> 1;
			if(!d)
				d = trgt_power - b->curr_power;
			b->curr_accel = d;
		} else {
			d = b->curr_accel;
		}
		b->curr_power += d;

		if(b->trgt_power == b->curr_power)
			b->curr_accel = 0;
	} else {
		// DIRECT MODE
		b->curr_power = b->trgt_power;
		b->curr_accel = 0;
	}

	// UPDATE BOOSTER OUTPUT
	digitalWrite(b->dirSignalPin, b->curr_power > 0);
	unsigned char pwmvalue = b->min_power + abs(b->curr_power);
	switch(b->pwmSignalPin) {
	case  3: OCR2B = pwmvalue; break;
	case  9: OCR1A = pwmvalue; break;
	case 10: OCR1B = pwmvalue; break;
	case 11: OCR2A = pwmvalue; break;
	default: cli.fatal("Cannot write PWM output to pin");
	}
}

void PwmMngr::refresh(void)
{
	for(int b = 0; b < nboosters; b++)
		booster_refresh(boosters + b);
}

void PwmMngr::accelerate(Booster *b, int v)
{
	b->trgt_power = v > 0
			? (b->trgt_power + v < b->max_power
				? b->trgt_power + v
				: b->max_power)
			: (b->trgt_power + v > -b->max_power
				? b->trgt_power + v
				: -b->max_power);
}

void PwmMngr::accelerate(int b, int v)
{
	if(b < 0 || b > nboosters)
		cli.fatal("pwmAccelerate: booster out of bounds.");

	accelerate(boosters + b, v);
}

void PwmMngr::speed(int b, int s)
{
	if(b < 0 || b > nboosters)
		cli.fatal("pwmSpeed: pwm booster out of bounds.");
	if(s < -255 || s > 255)
		cli.fatal("pwmSpeed: speed out of bounds.");
	boosters[b].trgt_power = s;
}

void PwmMngr::stop(int b)
{
  speed(b, 0);
}

void PwmMngr::switch_dir(int b)
{
  if(b < 0 || b > nboosters)
    cli.fatal("switch_dir: pwm out of bounds.");
    
  boosters[b].trgt_power = 0 - boosters[b].trgt_power;
}

