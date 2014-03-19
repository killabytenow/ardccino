/*****************************************************************************
 * Ardsim.h
 *
 * Arduino simulation library
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

#ifndef __ARDSIM_H__
#define __ARDSIM_H__

#include <math.h>
#include <string.h>

#include <StdSerial.h>

typedef unsigned char      uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int       uint32_t;
typedef uint8_t            boolean;
typedef uint8_t            byte;
typedef uint16_t           word;

#define bitmapdatatype uint16_t *
#define regsize        uint32_t
#define regtype        volatile uint32_t

#define swap(t,a,b)                                 \
		{                                   \
			register t __swap__temp__c; \
			__swap__temp__c = b;        \
			b = a;                      \
			a = __swap__temp__c;        \
		}
#define cbi(x,y)
#define sbi(x,y)
#define pgm_read_byte(x)	(*((byte *) (x)))
#define pgm_read_word(x)	(*((word *) (x)))
#define pgm_read_ptr(x)         (*((void **) (x)))

#define strcpy_P(x, y)		strcpy((x), (y))

#define fontbyte(x) cfont.font[x]  

#define pinMode(x, y)
#define digitalWrite(x, y)
#define delay(x)		usleep((x) * 1000)
#define PROGMEM

#endif
