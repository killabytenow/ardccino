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
#   (C) 2013-2015 Gerardo García Peña <killabytenow@gmail.com>
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
export ARDMK_DIR              = $(ARDUINO_DIR)
export AVR_TOOLS_DIR          = /usr
export TARGET                 = Ardccino
#export ARDUINO_LIBS           = UTFT
export BOARD_TAG              = mega2560
#export BOARD_TAG              = uno
#export MCU                    = atmega328p
#export F_CPU                  = 16000000
export ARDUINO_PORT           = /dev/$(shell \
					dmesg \
					| grep 'FTDI USB Serial Device converter now attached to\|: USB ACM device' \
					| sed 's/^.* \(tty[^:]*\).*$$/\1/' \
					| tail -n1)
export AVRDUDE_ARD_BAUDRATE   = 115200
#export AVRDUDE_ARD_PROGRAMMER = arduino
export AVRDUDE_ARD_PROGRAMMER = wiring
export USER_LIB_PATH          = ./
export MONITOR_BAUDRATE       = 115200
#export ARD_RESET_OPTS         = $(ARDUINO_PORT)

export EXTRA_CFLAGS=-Wall -Wextra
export EXTRA_CXXFLAGS=-Wall -Wextra

AUTO_FILES=\
	auto_banner.h		\
	auto_banner_wide.h	\
	auto_build_date.h	\
	auto_clierrs.h		\
	auto_clihelp.h		\
	auto_tokens.h

.PHONY : simulator real clean clean-simulator clean-real depends reset raw_upload show_boards monitor .FORCE

all : $(AUTO_FILES)
	$(MAKE) -f Real.mk

simulator : $(AUTO_FILES)
	$(MAKE) -f Simulator.mk

auto_tokens.h : tokens.list gen_code.pl
	./gen_code.pl tokens

auto_clierrs.h : clierrs.list gen_code.pl
	./gen_code.pl clierrs

auto_banner.h : banner.txt gen_code.pl
	./gen_code.pl banner

auto_banner_wide.h : banner_wide.txt gen_code.pl
	./gen_code.pl banner_wide

auto_clihelp.h : clihelp.txt gen_code.pl
	./gen_code.pl clihelp

auto_build_date.h : .FORCE
	./gen_code.pl build_date

clean-simulator :
	$(MAKE) -f Simulator.mk clean

clean-real :
	$(MAKE) -f Real.mk clean

clean : clean-simulator clean-real
	rm -f -- $(AUTO_FILES)

depends :
	$(MAKE) -f Simulator.mk depends
	$(MAKE) -f Real.mk depends

reset :
	$(MAKE) -f Real.mk reset

upload :
	$(MAKE) -f Real.mk upload

do_upload :
	$(MAKE) -f Real.mk do_upload

raw_upload :
	$(MAKE) -f Real.mk raw_upload

show_boards :
	$(MAKE) -f Real.mk show_boards

monitor :
	$(MAKE) -f Real.mk monitor

