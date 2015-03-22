/*****************************************************************************
 * pwm.cpp
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

#include "config.h"
#include "pwm.h"
#include "interrupts.h"

static int get_timer(uint8_t pin)
{
	uint8_t timer = pgm_read_byte_near(digital_pin_to_timer_PGM + pin);

	switch(timer) {
	case TIMER1A:
	case TIMER1B:
		timer = (1 << 1);
		break;
	case TIMER2:
	case TIMER2A:
	case TIMER2B:
		timer = (1 << 2);
		break;
#if defined (__AVR_ATmega2560__)
	case TIMER3A:
	case TIMER3B:
	case TIMER3C:
		timer = (1 << 3);
		break;
#endif
	case NOT_ON_TIMER:
		cli.fatal("get_timer(): Pin %d cannot be used for PWM output.", pin);
	default:
		cli.fatal("get_timer(): Pin %d uses unknown timer (%d)", pin, timer);
	}
	return timer;
}

static void enable_timer_pin(uint8_t pin)
{
	uint8_t timer = pgm_read_byte_near(digital_pin_to_timer_PGM + pin);

	switch(timer) {
	case TIMER1A: TCCR1A |= 0b10000000; break; // Clear OC1A on cmp match
	case TIMER1B: TCCR1A |= 0b00100000; break; // Clear OC1B on cmp match
#if defined(__AVR_ATmega8__)
	case TIMER2:  TCCR2  |= 0b10000000; break; // Clear OC2  on cmp match
#endif
	case TIMER2A: TCCR2A |= 0b10000000; break; // Clear OC2A on cmp match
	case TIMER2B: TCCR2A |= 0b00100000; break; // Clear OC2B on cmp match
#if defined (__AVR_ATmega2560__)
	case TIMER3A: TCCR3A |= 0b10000000; break; // Clear OC3A on cmp match
	case TIMER3B: TCCR3A |= 0b00100000; break; // Clear OC3B on cmp match
	case TIMER3C: TCCR3A |= 0b00001000; break; // Clear OC3C on cmp match
#endif
	case NOT_ON_TIMER:
		cli.fatal("enable_timer_pin(): Pin %d cannot be used for PWM output.", pin);
	default:
		cli.fatal("enable_timer_pin(): Pin %d uses unknown timer (%d)", pin, timer);
	}
}

static void set_timer_register(uint8_t pin, uint8_t pwm)
{
	uint8_t timer = pgm_read_byte_near(digital_pin_to_timer_PGM + pin);

#ifndef SIMULATOR
	switch(timer) {
	case TIMER1A: OCR1A = pwm; break;
	case TIMER1B: OCR1B = pwm; break;
#if defined(__AVR_ATmega8__)
	case TIMER2:  OCR2  = pwm; break;
#endif
	case TIMER2A: OCR2A = pwm; break;
	case TIMER2B: OCR2B = pwm; break;
#if defined (__AVR_ATmega2560__)
	case TIMER3A: OCR3A = pwm; break;
	case TIMER3B: OCR3B = pwm; break;
	case TIMER3C: OCR3C = pwm; break;
#endif
	default: cli.fatal("Cannot write PWM output to pin %d", pin);
	}
#endif
}

void PwmMngr::init(void)
{
	uint8_t timers = 0;

	cli.debug("Initializing PWM.");

	// discover which timers must be configured
	for(int b = 0; b < BoosterMngr::nboosters; b++)
		timers |= get_timer(BoosterMngr::boosters[b].pwmSignalPin);
	cli.debug("  timers %x", timers);

	disable_interrupts();

	// CONFIGURE TIMERS
#ifndef SIMULATOR
	if(timers & (1 << 1)) {
		// timer 1
		TCCR1A = (TCCR1A & 0b00001100)
			              // 0b00______ Normal port operation, OC1A disconnected
			              // 0b__00____ Normal port operation, OC1B disconnected
			              // 0b____**__ Reserved
			| 0b00000001; // 0b______01 Fast PWM 8-bit (!WGM13, WGM12, !WGM11, WGM10)
		TCCR1B = (TCCR1B & 0b11100000)
			              // 0b***_____ ICNC1, ICES1, Reserved
			| 0b00001000  // 0b___01___ Fast PWM 8-bit (!WGM13, WGM12, !WGM11, WGM10)
			| 0x04;       // 0b_____100 clock prescaler 0x04 (16e6/256 = 62.5 khz)
		TIMSK1 &= 0b11011000; // disable all timer 1 interrupts
		                      // 0b**_**___ Reserved
				      // 0b__0_____ input capture interrupt enable
				      // 0b_____0__ OCIE1B: output compare B match interrupt enable
				      // 0b______0_ OCIE1A: output compare A match interrupt enable
				      // 0b_______0 TOIE1: overflow interrupt enable
		OCR1A = OCR1B = 0;
	}

	if(timers & (1 << 2)) {
		// timer 2
		TCCR2A = (TCCR2A & 0b00001100)
			              // 0b00______ Normal port operation, OC2A disconnected
			              // 0b__00____ Normal port operation, OC2B disconnected
			              // 0b____**__ reserved
			| 0b00000011; // 0b00000011 Fast PWM (!WGM22, WGM21, WGM20)
		TCCR2B = (TCCR2B & 0b11110000)
			               // 0b****____ FOC2A, FOC2B, Reserved, Reserved
			| 0b00000000   // 0b____0___ Fast PWM (!WGM22, WGM21, WGM20)
			| 0x6;         // 0b_____110 clock prescaler 0x6 (16e6/256 = 62.5 khz)
		TIMSK2 &= 0b11111000;  // disable all timer 2 interrupts
		OCR2A = OCR2B = 0;
	}

#if defined (__AVR_ATmega2560__)
	if(timers & (1 << 3)) {
		// timer 1
		TCCR3A = (TCCR3A & 0b00000000)
			              // 0b00______ Normal port operation, OC3A disconnected
			              // 0b__00____ Normal port operation, OC3B disconnected
			              // 0b____00__ Normal port operation, OC3C disconnected
			| 0b00000001; // 0b______01 Fast PWM 8-bit (!WGM33, WGM32, !WGM31, WGM30)
		TCCR3B = (TCCR3B & 0b11100000)
			              // 0b***_____ ICNC3, ICES3, Reserved
			| 0b00001000  // 0b___01___ Fast PWM 8-bit (!WGM33, WGM32, !WGM31, WGM30)
			| 0x04;       // 0b_____100 clock prescaler 0x04 (16e6/256 = 62.5 khz)
		TIMSK3 &= 0b11010000; // disable all timer 1 interrupts
		                      // 0b**_*____ Reserved
				      // 0b__0_____ input capture interrupt enable
				      // 0b____0___ OCIE3C: output compare B match interrupt enable
				      // 0b_____0__ OCIE3B: output compare B match interrupt enable
				      // 0b______0_ OCIE3A: output compare A match interrupt enable
				      // 0b_______0 TOIE3: overflow interrupt enable
		OCR3A = OCR3B = OCR3C = 0;
	}
#endif

	// CONFIGURE OUTPUT PINS FOR DIRECT PWM OUTPUT
	for(int b = 0; b < BoosterMngr::nboosters; b++)
		enable_timer_pin(BoosterMngr::boosters[b].pwmSignalPin);
#endif

	//-------------------------------------------
	// RESET PWM STATUS
	for(int b = 0; b < BoosterMngr::nboosters; b++)
		BoosterMngr::boosters[b].reset();

	enable_interrupts();

	for(int b = 0; b < BoosterMngr::nboosters; b++)
		BoosterMngr::boosters[b].on();
}

void PwmMngr::fini(void)
{
	cli.debug("Disabling PWM");

	for(int b = 0; b < BoosterMngr::nboosters; b++)
		BoosterMngr::boosters[b].off();

	disable_interrupts();

	// disconnect all OC1A:B/OC2A:B pins and go to normal operations mode
#ifndef SIMULATOR
	// timer 1
	TCCR1A = TCCR1A & 0b00001100; TCCR1B = TCCR1B & 0b11100000;
	OCR1A = OCR1B = 0;
	TIMSK1 &= 0b11011000;
	// timer 2
	TCCR2A = TCCR2A & 0b00001100; TCCR2B = TCCR2B & 0b11110000;
	OCR2A = OCR2B = 0;
	TIMSK2 &= 0b11111000;
#if defined (__AVR_ATmega2560__)
	// timer 3
	TCCR3A = TCCR3A & 0b00000000; TCCR3B = TCCR3B & 0b11100000;
	OCR3A = OCR3B = OCR3C = 0;
	TIMSK3 &= 0b11010000;
#endif
#endif

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
	cli.debug("booster#%s power %d accel %d", b->name, b->curr_power, b->curr_accel);

	// UPDATE BOOSTER OUTPUT
	digitalWrite(b->dirSignalPin, b->curr_power > 0);
	unsigned char pwmvalue = b->min_power > abs(b->curr_power)
					? b->min_power
					: abs(b->curr_power);
	set_timer_register(b->pwmSignalPin, pwmvalue);
}

void PwmMngr::refresh(void)
{
	for(int b = 0; b < BoosterMngr::nboosters; b++)
		booster_refresh(BoosterMngr::boosters + b);
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
	if(b < 0 || b > BoosterMngr::nboosters)
		cli.fatal("pwmAccelerate: booster out of bounds.");

	accelerate(BoosterMngr::boosters + b, v);
}

void PwmMngr::speed(int b, int s)
{
	if(b < 0 || b > BoosterMngr::nboosters)
		cli.fatal("pwmSpeed: pwm booster out of bounds.");
	if(s < -255 || s > 255)
		cli.fatal("pwmSpeed: speed out of bounds.");
	BoosterMngr::boosters[b].trgt_power = s;
}

void PwmMngr::stop(int b)
{
	speed(b, 0);
}

void PwmMngr::switch_dir(int b)
{
	if(b < 0 || b > BoosterMngr::nboosters)
		cli.fatal("switch_dir: pwm out of bounds.");
    
	BoosterMngr::boosters[b].trgt_power = 0 - BoosterMngr::boosters[b].trgt_power;
}

