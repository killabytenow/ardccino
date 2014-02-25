###############################################################################
# Makefile
#
# Don't call directly. Use 
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
#   make simulator   - build a GTK simulator
#
# ---------------------------------------------------------------------------
# ardccino - Arduino dual PWM/DCC controller
#   (C) 2013-2014 Gerardo García Peña <killabytenow@gmail.com>
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

export ARDUINO_DIR            = /usr/share/arduino/
export ARDMK_DIR              = /usr
export AVR_TOOLS_DIR          = /usr
export TARGET                 = Ardccino
export ARDUINO_LIBS           = UTFT
export BOARD_TAG              = mega2560
#export MCU                    = atmega328p
#export F_CPU                  = 16000000
export ARDUINO_PORT           = /dev/$(shell dmesg |grep 'FTDI USB Serial Device converter now attached to' | sed 's/^.*attached to //' | tail -n1)
export AVRDUDE_ARD_BAUDRATE   = 115200
#export AVRDUDE_ARD_PROGRAMMER = arduino
export AVRDUDE_ARD_PROGRAMMER = wiring
export USER_LIB_PATH          = ./
export MONITOR_BAUDRATE       = 115200
export ARD_RESET_OPTS         = $(ARDUINO_PORT)

all :
	$(MAKE) -f Real.mk

clean :
	$(MAKE) -f Simulator.mk clean
	$(MAKE) -f Real.mk clean

depends :
	$(MAKE) -f Simulator.mk depends
	$(MAKE) -f Real.mk depends

reset :
	$(MAKE) -f Real.mk reset

raw_upload :
	$(MAKE) -f Real.mk raw_upload

show_boards :
	$(MAKE) -f Real.mk show_boards

monitor :
	$(MAKE) -f Real.mk monitor

simulator :
	$(MAKE) -f Simulator.mk

.PHONY : simulator

