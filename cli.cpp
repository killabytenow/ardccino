///////////////////////////////////////////////////////////////////////////////
// CLI
///////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "booster_mngr.h"
#include "cli.h"

void _ansi_cls(void)
{
  Serial.print("\x1B[1J");
  Serial.print("\x1B[1;1f");
}

void _ansi_goto(int x, int y)
{
  Serial.print("\x1B[");
  Serial.print(y);
  Serial.print(';');
  Serial.print(x);
  Serial.print('f');
}

///////////////////////////////////////////////////////////////////////////////
// ACTIONS
///////////////////////////////////////////////////////////////////////////////

PROGMEM const char cli_about_str_0[]  = "+--- DUAL PWM/DCC CONTROLLER v1.0 ------------------------------------+";
PROGMEM const char cli_about_str_1[]  = "|                                                                     |";
PROGMEM const char cli_about_str_2[]  = "|  PPPPPP  WW         WW MM       MM    // DDDDDD     CCCCCC  CCCCCC  |";
PROGMEM const char cli_about_str_3[]  = "|  PP   PP WW         WW MMMM   MMMM   //  DD   DD   CC      CC       |";
PROGMEM const char cli_about_str_4[]  = "|  PP   PP WW    W    WW MM MM MM MM   //  DD    DD CC      CC        |";
PROGMEM const char cli_about_str_5[]  = "|  PPPPPP  WW   WWW   WW MM  MMM  MM   //  DD    DD CC      CC        |";
PROGMEM const char cli_about_str_6[]  = "|  PP       WW WW WW WW  MM   M   MM  //   DD    DD CC      CC        |";
PROGMEM const char cli_about_str_7[]  = "|  PP       WWWW  WWWW   MM       MM  //   DD   DD   CC      CC       |";
PROGMEM const char cli_about_str_8[]  = "|  PP        WW    WW    MM       MM //    DDDDDD     CCCCCC  CCCCCC  |";
PROGMEM const char cli_about_str_9[]  = "|                                                                     |";
PROGMEM const char cli_about_str_10[] = "+------------------------------------ DUAL PWM/DCC CONTROLLER v1.0 ---+";
PROGMEM const char cli_about_str_11[] = "(C) 2013 Gerardo García Peña <killabytenow@gmail.com>"                  ;
PROGMEM const char cli_about_str_12[] = "This program is free software; you can redistribute it and/or modify it";
PROGMEM const char cli_about_str_13[] = "under the terms of the GNU General Public License as published by the"  ;
PROGMEM const char cli_about_str_14[] = "Free Software Foundation; either version 3 of the License, or (at your" ;
PROGMEM const char cli_about_str_15[] = "option) any later version."                                             ;
PROGMEM const char cli_about_str_16[] = "This program is distributed in the hope that it will be useful, but"    ;
PROGMEM const char cli_about_str_17[] = "WITHOUT ANY WARRANTY; without even the implied warranty of"             ;
PROGMEM const char cli_about_str_18[] = "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU"      ;
PROGMEM const char cli_about_str_19[] = "General Public License for more details."                               ;
PROGMEM const char cli_about_str_20[] = "You should have received a copy of the GNU General Public License along";
PROGMEM const char cli_about_str_21[] = "with this program; if not, write to the Free Software Foundation, Inc.,";
PROGMEM const char cli_about_str_22[] = "51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA"             ;

PROGMEM const char * const cli_about_str[] = {
  cli_about_str_0,  cli_about_str_1,  cli_about_str_2,  cli_about_str_3,
  cli_about_str_4,  cli_about_str_5,  cli_about_str_6,  cli_about_str_7,
  cli_about_str_8,  cli_about_str_9,  cli_about_str_10, cli_about_str_11,
  cli_about_str_12, cli_about_str_13, cli_about_str_14, cli_about_str_15,
  cli_about_str_16, cli_about_str_17, cli_about_str_18, cli_about_str_19,
  cli_about_str_20, cli_about_str_21, cli_about_str_22,
};

void cliAbout(void)
{
  char buffer[100];

  for(int i = 0; i < sizeof(cli_about_str) / sizeof(char *); i++) {
    strcpy_P(buffer, (char *) pgm_read_word(&(cli_about_str[i])));
    Serial.println(buffer);
  }
}

///////////////////////////////////////////////////////////////////////////////
// COMMAND LINE PARSER
///////////////////////////////////////////////////////////////////////////////

struct {
  char *token;
  int   value;
} cli_tokens[] = {
	"about",        CLI_TOKEN_ABOUT,
	"acceleration", CLI_TOKEN_ACCELERATION,
	"acked",        CLI_TOKEN_ACKED,
	"booster",      CLI_TOKEN_BOOSTER,
	"dcc",          CLI_TOKEN_DCC,
	"direct",       CLI_TOKEN_DIRECT,
	"inertial",     CLI_TOKEN_INERTIAL,
	"list",         CLI_TOKEN_LIST,
	"maximum",      CLI_TOKEN_MAXIMUM,
	"minimum",      CLI_TOKEN_MINIMUM,
	"mode",         CLI_TOKEN_MODE,
	"off",          CLI_TOKEN_OFF,
	"on",           CLI_TOKEN_ON,
	"paged",        CLI_TOKEN_PAGED,
	"passthrought", CLI_TOKEN_PASSTHROUGHT,
	"pwm",          CLI_TOKEN_PWM,
	"read",         CLI_TOKEN_READ,
	"send",         CLI_TOKEN_SEND,
	"speed",        CLI_TOKEN_SPEED,
	"stateful",     CLI_TOKEN_STATEFUL,
	"status",       CLI_TOKEN_STATUS,
};

int cli_token(char *token)
{
}

int cli_parse(char *buffer)
{
  char *p, *token[10], ntokens;
  int b, a;

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
    return 0;

  switch(cli_token(token[0])) {
  	case CLI_TOKEN_ABOUT:
		if(ntokens > 1) goto too_many_params;
		cli_about();
		break;

	case CLI_TOKEN_OFF:
	case CLI_TOKEN_PWM:
		if(ntokens > 1) goto too_many_params;
		boosterMngrSelectByName(token[0]);
		break;

	case CLI_TOKEN_DCC:
		if(ntokens == 1) {
			boosterMngrSelectByName(token[0]);
			break;
		}
		switch(cli_token(
	
  if(ntokens == 1) {
      if(!strcasecmp(token[0], "about")) { // [ALL] about
        cliAbout();
      } else 
      if(!strcasecmp(token[0], "off"  )
      || !strcasecmp(token[0], "pwm"  )
      || !strcasecmp(token[0], "dcc"  )) { // [ALL] off / pwm / dcc
        Serial.print("Selected mode ["); Serial.print(token[0]); Serial.println("]");
      } else
        goto unknown_command;
  } else
  if(

    case 2:
      // [ALL] booster list
      if(!strcasecmp(token[0], "booster")
      && !strcasecmp(token[1], "list"))
        return cli_booster_list();
      break;

    case 3:
      // [ALL] booster <n> status
      if(!strcasecmp(token[0], "booster")
      && !strcasecmp(token[2], "status")) {
        sscanf(token[1], "%d", &b);
        return cli_booster_status(b);
      }

      // [DCC] dcc mode (stateless|stateful)
      if(!strcasecmp(token[0], "dcc")
      && !strcasecmp(token[1], "mode")) {
        if(strcasecmp(booster_mngr[booster_mngr_selected].name, "dcc"))
          goto bad_mode;
        a = !strcmpcase(token[2], "stateful");
        sscanf(token[1], "%d", &b);
        return cli_booster_status(b);
      }
      break;

    case 4:
      // [ALL] booster <n> ...
      if(!strcasecmp(token[0], "booster")) {
        sscanf(token[1], "%d", &b);

        // [ALL] booster <n> power (off|on)
        if(!strcasecmp(token[2], "power")) {
          if(strcasecmp(token[3], "on") && strcasecmp(token[3], "off"))
            goto bad_syntax;
          return cli_booster_status(b);
        }

        // [PWM] booster <n> ...
        if(strcasecmp(booster_mngr[booster_mngr_selected].name, "pwm"))
          goto bad_mode;

        // [PWM] booster <n> speed [<v>]
        if(!strcasecmp(token[2], "speed")) {
          sscanf(token[3], "%d", &a);
          return cli_booster_status(b);
        }

        // [PWM] booster <n> mode (direct|inertial)
        if(!strcasecmp(token[2], "mode")) {
          a = !strcasecmp(token[3], "inertial");
          return cli_booster_status(b);
        }

        // [PWM] booster <n> acceleration [<a>]
        if(!strcasecmp(token[2], "acceleration")) {
          sscanf(token[3], "%d", &a);
          return cli_booster_status(b);
        }

      }
      break;

    case 5:
      // [ALL] booster <n> ...
      if(!strcasecmp(token[0], "booster")) {
        sscanf(token[1], "%d", &b);

        // [PWM] booster <n> ...
        if(strcasecmp(booster_mngr[booster_mngr_selected].name, "pwm"))
          goto bad_mode;

        // [PWM] booster <n> minimum speed [<v>]
        if(!strcasecmp(token[2], "minimum")
        && !strcasecmp(token[3], "speed")) {
          sscanf(token[4], "%d", &a);
          return cli_booster_status(b);
        }

        // [PWM] booster <n> maximum acceleration [<a>]
        if(!strcasecmp(token[2], "maximum")
        && !strcasecmp(token[3], "acceleration")) {
          sscanf(token[4], "%d", &a);
          return cli_booster_status(b);
        }
      }
      break;

    default:
      // [DCC] dcc send [service [acked]] command <bytes...>
      // [DCC] dcc read <v> [mode (paged|direct)]
  }

  return 0;

unknown_command:
	Serial.println("error: Unknown command");
	return 1;

too_many_params:
	Serial.println("error: too many parameters");
	return 1;

bad_syntax:
	Serial.println("error: Bad syntax.");
	return 1;

bad_mode:
	Serial.println("error: This command cannot be executed in current mode.");
	return 1;
}

void cliHandler(void)
{
}

void (*current_ui_handler)(void);


