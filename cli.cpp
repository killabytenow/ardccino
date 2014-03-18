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
	input_reading = false;
}

void Cli::init(void)
{
	Serial.begin(CLI_SERIAL_SPEED);
	Serial.println("Initializing");
	
	// enable "Set cursor key to application" DECCKM mode
	//   This mode forbids user to move cursor freely along the screen :D
	Serial.print(ANSI_CSI "?1h");
}

void Cli::_msg(const char *prefix, const char *frmt, va_list args)
{
	Serial.print(ANSI_EL(2) "\r");
	if(prefix)
		Serial.print(prefix);

	for(const char *p = frmt; *p; p++) {
		if(*p == '%') {
			switch(*++p) {
			case '\0':
				Serial.print('%');
				break;
			case 'd':
				Serial.print((int) va_arg(args, int));
				break;
			case 's':
				Serial.print((char *) va_arg(args, char *));
				break;
			case 'c':
				Serial.print((char) va_arg(args, int));
				break;
			case 'x':
				{
					unsigned int n = va_arg(args, unsigned int), x;
					int m;
					for(m = 16 - 4; m >= 0; m -= 4) {
						x = ((unsigned) (n >> m) & (unsigned) 0x000f);
						Serial.print((char) (x < 0x0a ? x + '0' : x + 'a' - 10));
					}
				}
				break;
			case 'p':
				{
					intptr_t n = va_arg(args, unsigned int), x;
					int m;
					for(m = 16 - 4; m >= 0; m -= 4) {
						x = ((intptr_t) (n >> m) & (intptr_t) 0x000f);
						Serial.print((char) (x < 0x0a ? x + '0' : x + 'a' - 10));
					}
				}
				break;
			default:
				Serial.print(*p);
			}
		} else
			Serial.print(*p);
	}
	if(input_reading) {
		Serial.print("\r\n" CLI_PROMPT);
		if(input_len > 0)
			Serial.print(input);
	} else
		Serial.println();
}

void Cli::debug(const char *frmt, ...)
{
	va_list args;
	va_start(args, frmt);
	_msg("#debug: ", frmt, args);
	va_end(args);
}

void Cli::info(const char *frmt, ...)
{
	va_list args;
	va_start(args, frmt);
	_msg(NULL, frmt, args);
	va_end(args);
}

void Cli::error(const char *frmt, ...)
{
	va_list args;
	va_start(args, frmt);
	_msg("ERROR: ", frmt, args);
	va_end(args);
}

void Cli::fatal(const char *frmt, ...)
{
	va_list args;
	off.enable();
	va_start(args, frmt);
	_msg("FATAL: ", frmt, args);
	va_end(args);
	while(1);
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
	if(input_len + 1 >= (signed) sizeof(input))
		return;
	input[input_len++] = c;
	input[input_len] = '\0';
	input_pos = 0;
	Serial.print(c);
}

void Cli::input_del(void)
{
	if(input_len <= 0)
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

	for(int i = 0; i < (signed) (sizeof(banner_wide) / sizeof(char *)); i++) {
		strcpy_P(buffer, (char *) pgm_read_word(&(banner_wide[i])));
		info(buffer);
	}
}

void Cli::booster_list(void)
{
	Booster *b;
	info("#"
		ANSI_SGR_BOLD "id" ANSI_SGR_BOLD_OFF "\t"
		ANSI_SGR_BOLD "enabled" ANSI_SGR_BOLD_OFF "\t"
		ANSI_SGR_BOLD "name" ANSI_SGR_BOLD_OFF);
	for(int bi = 0; bi < BoosterMngr::nboosters(); bi++) {
		b = BoosterMngr::booster(bi);
		info("%d\t%s\t%s",
			bi,
			b->enabled ? "yes" : "no",
			b->name);
	}
}

void Cli::booster_status(Booster *b)
{
	info("Booster [%s] is %s", b->name, b->enabled ? "ON" : "OFF");
	info("hw-config:");
	info("  pwmSignalPin: %d", b->pwmSignalPin);
	info("  dirSignalPin: %d", b->dirSignalPin);
	info("  tmpAlarmPin:  %d", b->tmpAlarmPin);
	info("  ocpAlarmPin:  %d", b->ocpAlarmPin);
	info("  rstSignalPin: %d", b->rstSignalPin);
	info("analog-control:");
	info("  trgt_power: %d", b->trgt_power);
	info("  curr_power: %d", b->curr_power);
	info("  curr_accel: %d", b->curr_accel);
	info("  inc_accel:  %d", b->inc_accel);
	info("  max_accel:  %d", b->max_accel);
	info("  min_power:  %d", b->min_power);
	info("  max_power:  %d", b->max_power);
	info("  mode:       %s", b->inertial ? "inertial" : "direct");
}

///////////////////////////////////////////////////////////////////////////////
// COMMAND LINE PARSER
///////////////////////////////////////////////////////////////////////////////

#define CLI_TOKEN_AT_LEAST(x)                                    \
		{                                                \
			if(ntokens < (x))                        \
				return CLI_ERR_NEED_MORE_PARAMS; \
		}
#define CLI_TOKEN_EXPECTED(x)                                    \
		{                                                \
			int8_t __x = (x);                        \
			if(ntokens < __x)                        \
				return CLI_ERR_NEED_MORE_PARAMS; \
			if(ntokens > __x)                        \
				return CLI_ERR_TOO_MANY_PARAMS;  \
		}

bool Cli::parse_integer(char *token, int *v)
{
	int n;
	int type;

	// identify number
	if(strlen(token) > 2
	&& token[0] == '0'
	&& (token[1] == 'x' || token[2] == 'X')) {
		// c/c++ hex number
		type = 1;
		token += 2;
	} else
	type = (strlen(token) > 1
		&& (token[strlen(token)-1] == 'h' || token[strlen(token)-1] == 'H'))
		? 2	// asm hex number
		: 0;	// decimal number

	// parse num
	n = 0;
	switch(type) {
	case 0: // decimal number
		for(; *token; token++) {
			if(*token < '0' || *token > '9') {
				return false;
			}
			n = (n * 10) + (*token - '0');
		}
		break;
	case 1: // c/c++ hex number
	case 2: // asm hex number
		for(; *token; token++) {
			if(*token >= '0' && *token <= '9')
				n = (n << 4) | (*token - '0');
			else
			if(*token >= 'a' && *token <= 'f')
				n = (n << 4) | (*token - 'a' + 10);
			else
			if(*token >= 'A' && *token <= 'F')
				n = (n << 4) | (*token - 'A' + 10);
			else
			break;
		}
		if((type != 2 && *token)
		|| (type == 2 && (*token != 'h' && *token != 'H')))
			return false;
		break;
	default:
		fatal("unknown number type");
	}
	if(v) *v = n;
	return true;
}

int Cli::parse_token(char *token, int *v)
{
	char buffer[CLI_TOKEN_MAX_LEN];
	for(int i = 0; i < (signed) (sizeof(cli_tokens) / sizeof(char *)); i++) {
		strcpy_P(buffer, (char *) pgm_read_word(&(cli_tokens[i])));
		if(!strcasecmp(token, buffer))
			return i;
	}
	if(parse_integer(token, v))
		return CLI_TOKEN_INTEGER;

	return -1;
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
	// COMMAND: booster <n> (status|info)
	case CLI_TOKEN_STATUS:
	case CLI_TOKEN_INFO:
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
	int r;
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

//debug("ntokens = %d", ntokens); for(int i = 0; i < ntokens; i++) debug("token[%d] = '%s'", i, token[i]);
	// [ALL] ---
	if(!ntokens)
		return;

	// execute command and print error/ok string
	r = ntokens < 10
		? execute(token, ntokens)
		: CLI_ERR_BAD_SYNTAX;

	// print error/ok string
	strcpy_P(err, (char *) pgm_read_word(&(cli_errs[r])));
	if(!r)
		info("%s", err);
	else
		error("%s", err);
}

void Cli::input_read(void)
{
	if(input_len < 0) {
		input_reading = true;
		Serial.print(CLI_PROMPT);
		input_len = 0;
	}

	while(Serial.available()) {
		char c = Serial.read();
ojete:
		switch(c) {
		case 13:
			Serial.println();
			input_reading = false;
			parse(input);
			input_reset();
			break;
		case 127: // backspace
			input_del();
			break;
		case 27:
			c = Serial.read();
			if(c != 79)
				goto ojete;
			c = Serial.read();
			switch(c) {
			case 65: // UP
				break;
			case 66: // DOWN
				break;
			case 68: // LEFT;
				break;
			case 67: // RIGHT
				break;
			default:
				goto ojete;
			}
			break;
		default:
			input_add(c);
		}
	}
}

#endif
