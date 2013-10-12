#!/bin/sh
###############################################################################
# clierrs.list
#
# Parse some input files and generate automated source code.
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

set -e

IFS="
"

#==============================================================================
# BUILD .tokens.h
#==============================================================================

N=0
OUT_H="auto_tokens.h"

cat<<EOF > "$OUT_H"
// ** THIS FILE IS AUTOGENERATED - DO NOT EDIT **

#include <Arduino.h>

// special tokens
#define CLI_TOKEN_INTEGER 100
#define CLI_TOKEN_STRING  101

EOF

for TOKEN in `grep -v '^ *\(#.*\)\?$' tokens.list`; do
	echo "#define CLI_TOKEN_`echo "$TOKEN" | tr a-z A-Z` $N"
	echo "PROGMEM const char cli_token_$N[] = \"$TOKEN\";"
	N=$(($N + 1))
done >> "$OUT_H"

echo "PROGMEM const char * const cli_tokens[] = {" >> "$OUT_H"
for i in `seq 0 $(($N - 1))`; do
	echo "	cli_token_$i,"
done >> "$OUT_H"
cat<<EOF >> "$OUT_H"
	NULL
};
EOF

#==============================================================================
# BUILD .clierrs.h
#==============================================================================

MAX_LEN=0
N=0
OUT_H="auto_clierrs.h"
cat<<EOF > "$OUT_H"
// ** THIS FILE IS AUTOGENERATED - DO NOT EDIT **

#include <Arduino.h>

EOF
for L in `grep -v '^ *\(#.*\)\?$' clierrs.list | sed 's# \+= \+#\t#'`; do
	echo "#define `echo "$L" | cut -f1` $N"
	M="`echo "$L" | cut -f2`"
	echo "PROGMEM const char cli_err_$N[] = \"$M\";"
	M="`echo -n "$M" | wc -c`"
	if [ "$M" -gt "$MAX_LEN" ]; then
		MAX_LEN="$M"
	fi
	N=$(($N + 1))
done >> "$OUT_H"
echo "#define CLI_ERRS_MAX_LEN $(($MAX_LEN + 1))" >> "$OUT_H"
echo "PROGMEM const char * const cli_errs[] = {" >> "$OUT_H"
for i in `seq 0 $(($N - 1))`; do
	echo "	cli_err_$i,"
done >> "$OUT_H"
echo "};" >> "$OUT_H"

#==============================================================================
# BUILD .banner_wide.h and .banner.h
#==============================================================================

for BANNER in banner_wide banner; do
	OUT_H="auto_$BANNER.h"
	N=0
	MAX_LEN=0

	echo "// ** THIS FILE IS AUTOGENERATED - DO NOT EDIT **"  > "$OUT_H"
	echo ""                                                  >> "$OUT_H"
	echo "#include <Arduino.h>"                              >> "$OUT_H"
	echo ""                                                  >> "$OUT_H"
	for L in `cat $BANNER.txt`; do
		echo "PROGMEM const char ${BANNER}_$N[] = \"$L\";"
		M="`echo -n "$L" | wc -c`"
		if [ "$M" -gt "$MAX_LEN" ]; then MAX_LEN="$M"; fi
		N=$(($N + 1))
	done >> "$OUT_H"
	echo "#define CLI_ERRS_MAX_LEN $(($MAX_LEN + 1))" >> "$OUT_H"
	echo "PROGMEM const char * const $BANNER[] = {" >> "$OUT_H"
	for i in `seq 0 $(($N - 1))`; do
		echo "	${BANNER}_$i,"
	done >> "$OUT_H"
	echo "};"    >> "$OUT_H"
done

exit 0
