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

#include "config.h"
#include "booster.h"
#include "dcc.h"
#include "pwm.h"
#include "off.h"
#include "cli.h"

#ifdef CLI_ENABLED

// following files are generated with 'gen_code.sh' script using the contents
// of 'tokens.list', 'clierrs.list' and 'banner_wide.txt' files.
#include "auto_banner_wide.h"
#include "auto_clierrs.h"
#include "auto_clihelp.h"
#include "auto_tokens.h"
#include "auto_build_date.h"

Cli::Cli(void)
{
	input_reset();
	input_reading = false;
}

void Cli::init(void)
{
	Serial.begin(CLI_SERIAL_SPEED);
	
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
			case 'b':
				Serial.print((bool) va_arg(args, int) ? "true" : "false");
				break;
			case 'd':
				{
				int x = va_arg(args, int);
				Serial.print(x);
				}
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
	_msg("! ", frmt, args);
	va_end(args);
}

void Cli::notice(const char *frmt, ...)
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

void Cli::_print_text(char **text, int l)
{
	char buffer[100];

	for(int i = 0; i < l; i++) {
		strcpy_P(buffer, (char *) pgm_read_ptr(&(text[i])));
		info(buffer);
	}
}

void Cli::about(void)
{
	char buffer[100];
	_print_text((char **) banner_wide, (signed) (sizeof(banner_wide) / sizeof(char *)));
	strcpy_P(buffer, (char *) build_date);
	info(buffer);
}

void Cli::help(void)
{
	_print_text((char **) clihelp, (signed) (sizeof(clihelp) / sizeof(char *)));
}

void Cli::booster_list(void)
{
	Booster *b;
	info(P("#"
		ANSI_SGR_BOLD "id" ANSI_SGR_BOLD_OFF "\t"
		ANSI_SGR_BOLD "enabled" ANSI_SGR_BOLD_OFF "\t"
		ANSI_SGR_BOLD "name" ANSI_SGR_BOLD_OFF));
	for(int bi = 0; bi < BoosterMngr::nboosters; bi++) {
		b = BoosterMngr::booster(bi);
		info(P("%d\t%s\t%s"),
			bi,
			b->enabled ? "yes" : "no",
			b->name);
	}
}

void Cli::booster_status(Booster *b)
{
	info(P("Booster [%s] is %s"), b->name, b->enabled ? "ON" : "OFF");
	info(P("hw-config:"));
	info(P("  pwmSignalPin: %d"), b->pwmSignalPin);
	info(P("  dirSignalPin: %d"), b->dirSignalPin);
	info(P("  tmpAlarmPin:  %d"), b->tmpAlarmPin);
	info(P("  ocpAlarmPin:  %d"), b->ocpAlarmPin);
	info(P("  rstSignalPin: %d"), b->rstSignalPin);
	info(P("analog-control:"));
	info(P("  trgt_power: %d"), b->trgt_power);
	info(P("  curr_power: %d"), b->curr_power);
	info(P("  curr_accel: %d"), b->curr_accel);
	info(P("  inc_accel:  %d"), b->inc_accel);
	info(P("  max_accel:  %d"), b->max_accel);
	info(P("  min_power:  %d"), b->min_power);
	info(P("  max_power:  %d"), b->max_power);
	info(P("  mode:       %s"), b->inertial ? "inertial" : "direct");
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

bool Cli::parse_unsigned(char *token, uint16_t *v)
{
	uint16_t n, type;

	// identify number
	if(strlen(token) > 2
	&& token[0] == '0'
	&& (token[1] == 'x' || token[2] == 'X')) {
		// c/c++ hex number (0x2222)
		type = 1;
		token += 2;
	} else
	type = (strlen(token) > 1
		&& (token[strlen(token)-1] == 'h' || token[strlen(token)-1] == 'H'))
		? 2	// asm hex number (2222h)
		: 0;	// decimal number (2222)

	// parse num
	n = 0;
	switch(type) {
	case 0: // decimal number
		for(; *token; token++) {
			if(*token < '0' || *token > '9')
				return false;
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
			if(type == 2 && (*token == 'h' || *token == 'H')) {
				if(*++token != 0)
					return false; // bad asm hex number
				break; // break loop
			} else
				return false;
		}
		if(*token)
			return false;
		break;
	default:
		fatal(P("unknown number type"));
	}

	if(v) *v = n;
	return true;
}

bool Cli::parse_signed(char *token, int16_t *v)
{
	int neg;

	// check negative
	if(*token == '-') {
		neg = -1;
		token++;
	} else {
		neg = 1;
	}

	if(!parse_unsigned(token, (uint16_t *) v))
		return false;

	if(v) *v *= neg;
	return true;
}

int Cli::parse_token(char *token)
{
	char buffer[CLI_TOKEN_MAX_LEN];
	unsigned l = strlen(token);
	for(int i = 0; i < (signed) (sizeof(cli_tokens) / sizeof(cli_token_t)); i++) {
		strcpy_P(buffer, (char *) pgm_read_ptr(&(cli_tokens[i].token)));
		if(l <= strlen(buffer)
		&& pgm_read_byte(&(cli_tokens[i].minlen)) <= l
		&& !strncasecmp(token, buffer, strlen(token)))
			return i;
	}
	return -1;
}

int Cli::parse_token(char *token, uint16_t *v)
{
	int t;
	if((t = parse_token(token)) >= 0)
		return t;
	if(parse_unsigned(token, v))
		return CLI_TOKEN_UINT;
	return -1;
}

int Cli::parse_token(char *token, int16_t *v)
{
	int t;
	if((t = parse_token(token)) >= 0)
		return t;
	if(parse_signed(token, v))
		return CLI_TOKEN_INT;
	return -1;
}

int Cli::execute_booster(char **token, char ntokens)
{
	uint16_t booster, ui;
	int16_t si;
	int t;

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
	if(t != CLI_TOKEN_UINT)
		return CLI_ERR_BAD_SYNTAX;
	if(BoosterMngr::booster(booster) == NULL)
		return CLI_ERR_BAD_BOOSTER;

	// here we need at least 3 tokens
	CLI_TOKEN_AT_LEAST(3);

	switch(parse_token(token[2])) {
	case CLI_TOKEN_RESET:
		CLI_TOKEN_EXPECTED(3);
		BoosterMngr::booster(booster)->reset();
		break;
	// COMMAND: booster <n> power (on|off)
	// COMMAND: booster <n> power <v>
	case CLI_TOKEN_POWER:
		CLI_TOKEN_EXPECTED(4);
		switch(parse_token(token[3], &si)) {
		case CLI_TOKEN_ON:
			BoosterMngr::current->on(booster);
			break;
		case CLI_TOKEN_OFF:
			BoosterMngr::current->off(booster);
			break;
		case CLI_TOKEN_INT:
			if(si < -255 || si > 255)
				return CLI_ERR_BAD_SYNTAX;
			if(!pwm.enabled())
				return CLI_ERR_BAD_MODE;
			pwm.speed(booster, si);
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
		switch(parse_token(token[3])) {
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
		if(parse_token(token[3], &ui) != CLI_TOKEN_UINT)
			return CLI_ERR_BAD_SYNTAX;
		if(!pwm.enabled())
			return CLI_ERR_BAD_MODE;
		BoosterMngr::booster(booster)->set_inc_accel(ui);
		break;
	// COMMAND: booster <n> minimum power [<v>]
	case CLI_TOKEN_MINIMUM:
		CLI_TOKEN_EXPECTED(5);
		if(parse_token(token[3]) != CLI_TOKEN_POWER)
			return CLI_ERR_BAD_SYNTAX;
		if(parse_token(token[4], &ui) != CLI_TOKEN_UINT)
			return CLI_ERR_BAD_SYNTAX;
		if(!pwm.enabled())
			return CLI_ERR_BAD_MODE;
		BoosterMngr::booster(booster)->set_min_power(ui);
		break;
	// COMMAND: booster <n> maximum (acceleration|power) [<a>]
	case CLI_TOKEN_MAXIMUM:
		CLI_TOKEN_EXPECTED(5);
		if(parse_token(token[4], &ui) != CLI_TOKEN_UINT)
			return CLI_ERR_BAD_SYNTAX;
		if(!pwm.enabled())
			return CLI_ERR_BAD_MODE;
		switch(parse_token(token[3])) {
		case CLI_TOKEN_ACCELERATION:
			BoosterMngr::booster(booster)->set_max_accel(ui);
			break;
		case CLI_TOKEN_POWER:
			BoosterMngr::booster(booster)->set_max_power(ui);
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

// COMMAND: <<trg_spec>> speed [4bit|5bit|7bit] [light (on|off)] [+-]<v> [acked]
int Cli::execute_dcc_cmd_speed(char **token, char ntokens, bool service_track, uint16_t address)
{
	uint16_t speed = 0;
	int st;
	bool light = false;

	if(ntokens < 1)
		return CLI_ERR_NEED_MORE_PARAMS;

	switch(st = parse_token(*token)) {
	case CLI_TOKEN_4BIT:
		ntokens--; token++;
		if(ntokens < 1)
			return CLI_ERR_NEED_MORE_PARAMS;
		if(parse_token(*token) == CLI_TOKEN_LIGHT) {
			ntokens--; token++;
			goto light;
		}
		break;

	case CLI_TOKEN_LIGHT:
		cli.info(P("'light' parameter forces deprecated 4bit speed mode."));
		st = CLI_TOKEN_4BIT;
		ntokens--; token++;
		goto light;

	case CLI_TOKEN_5BIT:
	case CLI_TOKEN_7BIT:
		ntokens--; token++;
		break;
		
	default:
#warning "TODO: get current deco speed and type"
		st = CLI_TOKEN_5BIT;
		break;
	}
	goto speed;

light:
	if(ntokens < 1)
		return CLI_ERR_NEED_MORE_PARAMS;
	switch(parse_token(*token)) {
	case CLI_TOKEN_ON:
		light = true;
		break;
	case CLI_TOKEN_OFF:
		light = false;
		break;
	default:
		return CLI_ERR_BAD_SYNTAX;
	}
	ntokens--; token++;

speed:
	if(ntokens < 1)
		return CLI_ERR_NEED_MORE_PARAMS;
	if(parse_token(*token, (int16_t *) &speed) != CLI_TOKEN_INT)
		return CLI_ERR_BAD_SYNTAX;

	switch(st) {
	case CLI_TOKEN_4BIT: speed = DCC_DECO_SPEED_GET4(speed, light); break;
	case CLI_TOKEN_5BIT: speed = DCC_DECO_SPEED_GET5(speed); break;
	case CLI_TOKEN_7BIT: speed = DCC_DECO_SPEED_GET7(speed); break;
	}
	if(speed == DCC_DECO_SPEED_BAD)
		return CLI_ERR_OUT_OF_BOUNDS;

	if(!dcc.set_speed(service_track, address, speed))
		return CLI_ERR_OP_FAILED;

	return CLI_ERR_OK;
}

// PENDING COMMANDS
// COMMAND: <<trg_spec>> f <f> (on|off) [acked]
// COMMAND: <<trg_spec>> analog <f> <v> [acked]
// COMMAND: <<trg_spec>> ack (service|railcom|off)
// COMMAND: <<trg_spec>> send [service] command <bytes...> [acked]
// COMMAND: <<trg_spec>> read <v> [mode (paged|direct)]
int Cli::execute_dcc_cmd(char **token, char ntokens)
{
	bool service_track;
	uint16_t address;

	if(!dcc.enabled())
		return CLI_ERR_BAD_MODE;

	// command addressed to service track?
	service_track = token[0][0] == '!';
	if(!token[0][1]) {
		token++;
		ntokens--;
	} else
		token[0]++;
	if(!ntokens)
		return CLI_ERR_NEED_MORE_PARAMS;

	// decide address type
	if(token[0][0] == '@') {
		// broadcast
		address = DCC_DECO_ADDR_BROADCAST;
		if(!token[0][1]) {
			token++;
			ntokens--;
		} else
			token[0]++;
	} else {
		int8_t address_t;
		switch(token[0][0]) {
		case '-':
			// 7bit forced
			address_t = DCC_ADDR_TYPE_7BIT;
			token[0]++;
			break;
		case '+':
			address_t = DCC_ADDR_TYPE_14BIT;
			token[0]++;
			break;
		default:
			address_t = DCC_ADDR_TYPE_NONE;
		}
		if(!parse_unsigned(*token, &address))
			return CLI_ERR_BAD_SYNTAX;
		address = dcc.get_address(address, address_t);
		token++;
		ntokens--;
	}
	if(address == DCC_DECO_ADDR_BAD)
		return CLI_ERR_OUT_OF_BOUNDS;

	// move to command and process it
	if(!ntokens)
		return CLI_ERR_NEED_MORE_PARAMS;

	// process dcc command
	switch(parse_token(*token)) {
	case CLI_TOKEN_SPEED:
		return execute_dcc_cmd_speed(token+1, ntokens-1, service_track, address);

	default:
		return CLI_ERR_NOT_IMPLEMENTED;
	}

	return CLI_ERR_OK;
}

// PENDING COMMANDS
// COMMAND: dcc mode (pass_through|stateful)
int Cli::execute_dcc(char **token, char ntokens)
{
	if(!dcc.enabled())
		return CLI_ERR_BAD_MODE;

	switch(parse_token(*token)) {
	case CLI_TOKEN_ADDRESS:
		if(ntokens < 2)
			return CLI_ERR_NEED_MORE_PARAMS;
		if(ntokens > 2)
			return CLI_ERR_TOO_MANY_PARAMS;
		switch(parse_token(token[1])) {
		case CLI_TOKEN_ADVANCED:
			cli.info(P("Using 4-digit (14bit) addresses by default."));
			dcc.default_addr_type = DCC_ADDR_TYPE_14BIT;
			break;
		case CLI_TOKEN_NORMAL:
			cli.info(P("Using 2-digit (7bit) addresses by default."));
			dcc.default_addr_type = DCC_ADDR_TYPE_7BIT;
			break;
		case CLI_TOKEN_AUTO:
			cli.info(P("Using auto-detect address algorithm."));
			dcc.default_addr_type = DCC_ADDR_TYPE_AUTO;
			break;
		default:
			return CLI_ERR_BAD_SYNTAX;
		}
		break;

	default:
		return CLI_ERR_NOT_IMPLEMENTED;
	}

	return CLI_ERR_OK;
}

int Cli::execute(char **token, char ntokens)
{
	switch(parse_token(token[0])) {
	// COMMAND: about
  	case CLI_TOKEN_ABOUT:
		if(ntokens > 1) return CLI_ERR_TOO_MANY_PARAMS;
		about();
		break;
	// COMMAND: help
  	case CLI_TOKEN_HELP:
		if(ntokens > 1) return CLI_ERR_TOO_MANY_PARAMS;
		help();
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
		if(ntokens > 1)
			return execute_dcc(token + 1, ntokens - 1);
		dcc.enable();
		break;
	// COMMANDS: booster ***
	case CLI_TOKEN_BOOSTER:
		return execute_booster(token, ntokens);
	// other
	default:
		// DIRECTED DCC MESSAGE
		if(token[0][0] == '@' || token[0][0] == '!')
			return execute_dcc_cmd(token, ntokens);
		return CLI_ERR_UNKNOWN_COMMAND;
	}

	return CLI_ERR_OK;
}

void Cli::parse(char *buffer)
{
	char *p, *token[10];
	int ntokens, r;
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
	strcpy_P(err, (char *) pgm_read_ptr(&(cli_errs[r])));
	if(!r)
		info(P("%s"), err);
	else
		error(P("%s"), err);
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
			if(Serial.available())
				Serial.read();
		case 10:
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
		case 9:
			break;
		default:
#ifdef SIMULATOR
			if(!(c >= 'a' && c <= 'z')
			&& !(c >= 'A' && c <= 'Z')
			&& !(c >= '0' && c <= '9')
			&& c != ' ' && c != '-' && c != '+'
			&& c != '?' && c != '@')
				SIM_DBG(P("puta=%d (%c)"), c, c);
#endif
			input_add(c);
		}
	}
}

#endif
