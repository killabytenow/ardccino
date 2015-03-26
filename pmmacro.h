/*****************************************************************************
 * pmmacro.h
 *
 * Kick strings to the PROGMEM, but offer a quick&easy way to restore them.
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

#ifndef __PMMACRO_H__
#define __PMMACRO_H__

#define P(str)	({						\
			strncpy_P(__p_buffer, PSTR(str), sizeof(__p_buffer) - 1);	\
			__p_buffer[sizeof(__p_buffer) - 1] = '\0';			\
			__p_buffer;				\
		})

#define __P_BUFFER_SIZE	128

extern char __p_buffer[__P_BUFFER_SIZE];

#endif
