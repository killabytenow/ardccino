/*****************************************************************************
 * cli.h
 *
 * Serial CLI remote/sub-human interface
 *
 * NOTE: If you have compilation errors related to the absence of '.tokens.h',
 *       '.clierrs.h' or '.banner_wide.h' includes, run the 'gen_code.sh'
 *       script.
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

#ifndef __CLI_H__
#define __CLI_H__

#include "config.h"

#ifdef CLI_ENABLED

class Cli {
	private:
		char input[255];
		int  input_len;
		int  input_pos;

		// command actions
		void about(void);
		void booster_list(void);
		void booster_status(Booster *b);

		// output
		void _msg(char *prefix, char *frmt, va_list args);

		// read input
		void input_add(char c);
		void input_del(void);
		void input_reset(void);

		// parsing and execution
		int parse_token(char *token, int *i);
		int parse_token(char *token);
		int execute_booster(char **token, char ntokens);
		int execute_dcc(char **token, char ntokens);
		int execute(char **token, char ntokens);
		void parse(char *buffer);

	public:
		Cli(void);
		void input_read(void);
		void debug(char *frmt, ...);
		void info(char *frmt, ...);
		void error(char *frmt, ...);
};

#endif

#endif
