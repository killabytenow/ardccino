/*****************************************************************************
 * cli.cpp
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

#include <Arduino.h>
#include "config.h"
#include "booster.h"
#include "dcc.h"
#include "pwm.h"
#include "off.h"
#include "cli.h"

#ifdef CLI_ENABLED

// following files are generated with 'gen_code.sh' script using the contents
// of 'tokens.list', 'clierrs.list' and 'banner_wide.txt' files.
#include "auto_tokens.h"
#include "auto_clierrs.h"
#include "auto_banner_wide.h"

Cli::Cli(void)
{
	input_reset();
}

void Cli::input_reset(void)
{
	*input = '\0';
	input_len = -1;
	input_pos = 0;
	//last_nbc = 0;
}

void Cli::input_add(char c)
{
	if(input_len + 1 < sizeof(input))
		return;
	input[input_len++] = c;
	input[input_len] = '\0';
	input_pos = 0;
	Serial.print(c);
}

void Cli::input_del(void)
{
	if(input_len > 0)
		return;
	input[--input_len] = '\0';
	input_pos = 0;
	Serial.print(ANSI_CUB(1) ANSI_ED(0));
}

///////////////////////////////////////////////////////////////////////////////
// ACTIONS
///////////////////////////////////////////////////////////////////////////////

void Cli::about(void)
{
	char buffer[100];

	for(int i = 0; i < sizeof(banner_wide) / sizeof(char *); i++) {
		strcpy_P(buffer, (char *) pgm_read_word(&(banner_wide[i])));
		Serial.println(buffer);
	}
}

void Cli::booster_list(void)
{
#warning "TODO"
}

void Cli::booster_status(Booster *b)
{
	Serial.print("Booster ["); Serial.print(b->name); Serial.print("] is ");
	Serial.println(b->enabled ? "ON" : "OFF");
	Serial.println("hw-config:");
	Serial.print("  pwmSignalPin: "); Serial.print(b->pwmSignalPin);
	Serial.print("  dirSignalPin: "); Serial.println(b->dirSignalPin);
	Serial.print("  tmpAlarmPin: "); Serial.print(b->tmpAlarmPin);
	Serial.print("  ocpAlarmPin: "); Serial.println(b->ocpAlarmPin);
	Serial.print("  rstSignalPin: "); Serial.println(b->rstSignalPin);
	Serial.print("analog-control:");
	Serial.print("  trgt_power: "); Serial.println(b->trgt_power);
	Serial.print("  curr_power: "); Serial.println(b->curr_power);
	Serial.print("  curr_accel: "); Serial.println(b->curr_accel);
	Serial.print("  inc_accel:  "); Serial.println(b->inc_accel);
	Serial.print("  max_accel:  "); Serial.println(b->max_accel);
	Serial.print("  min_power:  "); Serial.println(b->min_power);
	Serial.print("  max_power:  "); Serial.println(b->max_power);
	Serial.print("  mode:       "); Serial.println(b->inertial ? "inertial" : "direct");
}

///////////////////////////////////////////////////////////////////////////////
// COMMAND LINE PARSER
///////////////////////////////////////////////////////////////////////////////

#define CLI_TOKEN_AT_LEAST(x)                                    \
		{                                                \
			if((x) < ntokens)                        \
				return CLI_ERR_NEED_MORE_PARAMS; \
		}
#define CLI_TOKEN_EXPECTED(x)                                    \
		{                                                \
			int8_t __x = (x);                        \
			if(__x < ntokens)                        \
				return CLI_ERR_NEED_MORE_PARAMS; \
			if(__x > ntokens)                        \
				return CLI_ERR_TOO_MANY_PARAMS;  \
		}

int Cli::parse_token(char *token, int *i)
{
}

int Cli::parse_token(char *token)
{
	return parse_token(token, NULL);
}

int Cli::execute_booster(char **token, char ntokens)
{
	int t, booster, a;

	// here we need at least 2 tokens
	CLI_TOKEN_AT_LEAST(2);
	t = parse_token(token[1], &booster);

	// COMMAND: booster list
	if(t == CLI_TOKEN_LIST) {
		CLI_TOKEN_EXPECTED(2);
		booster_list();
		return CLI_ERR_OK;
	}

	// COMMAND: booster <n> ***
	if(t != CLI_TOKEN_INTEGER)
		return CLI_ERR_UNKNOWN_COMMAND;
	if(BoosterMngr::booster(booster) == NULL)
		return CLI_ERR_BAD_BOOSTER;

	// here we need at least 3 tokens
	CLI_TOKEN_AT_LEAST(3);

	switch(parse_token(token[2], NULL)) {
	case CLI_TOKEN_RESET:
		CLI_TOKEN_EXPECTED(3);
		BoosterMngr::booster(booster)->reset();
		break;
	// COMMAND: booster <n> power (on|off)
	// COMMAND: booster <n> power <v>
	case CLI_TOKEN_POWER:
		CLI_TOKEN_EXPECTED(4);
		switch(parse_token(token[3], &a)) {
		case CLI_TOKEN_ON:
			BoosterMngr::booster(booster)->on();
			break;
		case CLI_TOKEN_OFF:
			BoosterMngr::booster(booster)->off();
			break;
		case CLI_TOKEN_INTEGER:
			if(a < -255 || a > 255)
				return CLI_ERR_BAD_SYNTAX;
			if(!pwm.enabled())
				return CLI_ERR_BAD_MODE;
			pwm.speed(booster, a);
			break;
		default:
			return CLI_ERR_BAD_SYNTAX;
		}
		break;
	// COMMAND: booster <n> status
	case CLI_TOKEN_STATUS:
		CLI_TOKEN_EXPECTED(3);
		booster_status(BoosterMngr::booster(booster));
		break;
	// COMMAND: booster <n> mode (direct|inertial)
	case CLI_TOKEN_MODE:
		CLI_TOKEN_EXPECTED(4);
		if(!pwm.enabled())
			return CLI_ERR_BAD_MODE;
		switch(parse_token(token[3], NULL)) {
		case CLI_TOKEN_DIRECT:
			BoosterMngr::booster(booster)->set_mode_direct();
			break;
		case CLI_TOKEN_INERTIAL:
			BoosterMngr::booster(booster)->set_mode_inertial();
			break;
		default:
			return CLI_ERR_BAD_SYNTAX;
		}
		break;
	// COMMAND: booster <n> acceleration [<a>]
	case CLI_TOKEN_ACCELERATION:
		CLI_TOKEN_EXPECTED(4);
		if(parse_token(token[3], &a) != CLI_TOKEN_INTEGER)
			return CLI_ERR_BAD_SYNTAX;
		if(!pwm.enabled())
			return CLI_ERR_BAD_MODE;
		BoosterMngr::booster(booster)->set_inc_accel(a);
		break;
	// COMMAND: booster <n> minimum power [<v>]
	case CLI_TOKEN_MINIMUM:
		CLI_TOKEN_EXPECTED(5);
		if(parse_token(token[3], NULL) != CLI_TOKEN_POWER)
			return CLI_ERR_BAD_SYNTAX;
		if(parse_token(token[4], &a) != CLI_TOKEN_INTEGER)
			return CLI_ERR_BAD_SYNTAX;
		if(!pwm.enabled())
			return CLI_ERR_BAD_MODE;
		BoosterMngr::booster(booster)->set_min_power(a);
		break;
	// COMMAND: booster <n> maximum (acceleration|power) [<a>]
	case CLI_TOKEN_MAXIMUM:
		CLI_TOKEN_EXPECTED(5);
		if(parse_token(token[4], &a) != CLI_TOKEN_INTEGER)
			return CLI_ERR_BAD_SYNTAX;
		if(!pwm.enabled())
			return CLI_ERR_BAD_MODE;
		switch(parse_token(token[3], NULL)) {
		case CLI_TOKEN_ACCELERATION:
			BoosterMngr::booster(booster)->set_max_accel(a);
			break;
		case CLI_TOKEN_POWER:
			BoosterMngr::booster(booster)->set_max_power(a);
			break;
		default:
			return CLI_ERR_BAD_SYNTAX;
		}
		break;
	default:
		return CLI_ERR_BAD_SYNTAX;
	}
	return CLI_ERR_OK;
}

// PENDING COMMANDS
// COMMAND: dcc mode (pass_through|stateful)
// COMMAND: dcc [!] <n> speed [4bit|5bit|7bit] [+-]<v> [acked]
// COMMAND: dcc [!] <n> f <f> (on|off) [acked]
// COMMAND: dcc [!] <n> analog <f> <v> [acked]
// COMMAND: dcc [!] <n> address (advanced|normal) [acked]
// COMMAND: dcc [!] <n> ack (service|railcom|off)
// COMMAND: dcc [!] <n> send [service] command <bytes...> [acked]
// COMMAND: dcc [!] <n> read <v> [mode (paged|direct)]
int Cli::execute_dcc(char **token, char ntokens)
{
	return CLI_ERR_NOT_IMPLEMENTED;
}

int Cli::execute(char **token, char ntokens)
{
	switch(parse_token(token[0], NULL)) {
	// COMMAND: about
  	case CLI_TOKEN_ABOUT:
		if(ntokens > 1) return CLI_ERR_TOO_MANY_PARAMS;
		about();
		break;
	// COMMAND: off
	case CLI_TOKEN_OFF:
		if(ntokens > 1) return CLI_ERR_TOO_MANY_PARAMS;
		if(off.enabled()) return CLI_ERR_MODE_ALREADY_ENABLED;
		off.enable();
		break;
	// COMMAND: pwm
	case CLI_TOKEN_PWM:
		if(ntokens > 1) return CLI_ERR_TOO_MANY_PARAMS;
		if(pwm.enabled()) return CLI_ERR_MODE_ALREADY_ENABLED;
		pwm.enable();
		break;
	// COMMAND:  dcc
	// COMMANDS: dcc ***
	case CLI_TOKEN_DCC:
		if(ntokens != 1)
			return execute_dcc(token, ntokens);
		dcc.enable();
		break;
	// COMMANDS: booster ***
	case CLI_TOKEN_BOOSTER:
		return execute_booster(token, ntokens);
	// other
	default:
		return CLI_ERR_UNKNOWN_COMMAND;
	}

	return CLI_ERR_OK;
}

void Cli::parse(char *buffer)
{
	char *p, *token[10], ntokens;
	int b, a, r;
	char err[CLI_ERRS_MAX_LEN];

	// tokenize
	ntokens = 0;
	p = buffer;
	while(*p && ntokens < 10) {
		while(*p && (*p == ' ' || *p == '\t')) p++;
		if(!*p)
		break;

		// delimit token
		token[ntokens++] = p;
		while(*p && *p != ' ' && *p != '\t') p++;
		if(!*p)
		break;
		*p++ = '\0';
	}

	// [ALL] ---
	if(!ntokens)
		return;

	// execute command and print error/ok string
	r = ntokens < 10
		? execute(token, ntokens)
		: CLI_ERR_BAD_SYNTAX;

	// print error/ok string
	strcpy_P(err, (char *) pgm_read_word(&(cli_errs[r])));
	Serial.println(err);
}

void Cli::input_read(void)
{
	if(input_len < 0) {
		Serial.println();
		Serial.print(CLI_PROMPT);
		input_pos = 0;
	}

	while(char c = Serial.read()) {
		switch(c) {
		//case '\t':
		case '\n':
			parse(input);
			input_len = -1;
			break;
		case '\b':
			input_del();
			break;
		default:
			input_add(c);
		}
	}
}

#endif
