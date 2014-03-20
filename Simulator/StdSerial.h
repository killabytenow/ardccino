/*****************************************************************************
 * ardccino.ino
 *
 * Arduino's Serial class emulator.
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

#include <stdio.h>

#ifndef __SERIAL_H__
#define __SERIAL_H__

#ifdef __cplusplus

class StdSerial
{
	public:
		int fd;

		StdSerial();
		void set_fd(int fd);
		void begin(int speed);
		void print(const char *str);
		void print(char c);
		void print(int c);
		void print(unsigned int c);
		void println(const char *str);
		void println(char c);
		void println(int i);
		void println(unsigned int i);
		void println(void);
		int available(void);
		int read(void);
};

extern StdSerial Serial;

#endif

#endif
