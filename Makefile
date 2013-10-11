###############################################################################
# Makefile
#
# OMFG!! A makefile!! A real makefile!!
#
#   make             - no upload
#   make clean       - remove all our dependencies
#   make depends     - update dependencies
#   make reset       - reset the Arduino by tickling DTR on the serial port
#   make raw_upload  - upload without first resetting
#   make show_boards - list all the boards defined in boards.txt
#   make monitor     - connect to the Arduino's serial port
#                      NOTE: If you want to change the baudrate, just set
#                            MONITOR_BAUDRATE. If you don't set it, it defaults
#                            to 9600 baud.
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
TARGET                 = Ardccino
ARDUINO_LIBS           = UTFT
BOARD_TAG              = mega2560
#MCU                    = atmega328p
#F_CPU                  = 16000000
ARDUINO_PORT           = /dev/$(shell dmesg |grep 'FTDI USB Serial Device converter now attached to' | sed 's/^.*attached to //' | tail -n1)
AVRDUDE_ARD_BAUDRATE   = 115200
#AVRDUDE_ARD_PROGRAMMER = arduino
AVRDUDE_ARD_PROGRAMMER = wiring
USER_LIB_PATH          = ./libraries/
MONITOR_BAUDRATE       = 115200
ARD_RESET_OPTS         = $(ARDUINO_PORT)

include $(ARDUINO_DIR)/Arduino.mk

auto_tokens.h auto_clierrs.h auto_banner_wide.h auto_banner.h : gen_code.sh tokens.list clierrs.list banner.txt banner_wide.txt
	./gen_code.sh

