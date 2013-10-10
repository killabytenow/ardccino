###############################################################################
# Makefile
#
# OMFG!! A makefile!! A real makefile!!
#
# ---------------------------------------------------------------------------
# ardccino - Arduino dual PWM/DCC controller
#   (C) 2013 Gerardo García Peña <killabytenow@gmail.com>
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the Free
#   Software Foundation; either version 3 of the License, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
#   or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
#   for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#
###############################################################################

ARDUINO_DIR            = /usr/share/arduino/
ARDMK_DIR              = /usr
AVR_TOOLS_DIR          = /usr
TARGET                 = "Ardccino"
ARDUINO_LIBS           = UTFT
BOARD_TAG              = mega2560
#MCU                    = atmega328p
#F_CPU                  = 16000000
ARDUINO_PORT           = /dev/`dmesg |grep 'FTDI USB Serial Device converter now attached to'|sed 's#^.*attached to ##'`
AVRDUDE_ARD_BAUDRATE   = 115200
AVRDUDE_ARD_PROGRAMMER = arduino
USER_LIB_PATH          = ./libraries/

%.h::
	touch $@

.tokens.h .clierrs.h : gen_code.sh tokens.list clierrs.list banner.txt banner_wide.txt
	./gen_code.sh

include $(ARDUINO_DIR)/Arduino.mk

