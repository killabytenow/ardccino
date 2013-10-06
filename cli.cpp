///////////////////////////////////////////////////////////////////////////////
// ANSI
///////////////////////////////////////////////////////////////////////////////

void ansi_cls(void)
{
  Serial.print("\x1B[1J");
  Serial.print("\x1B[1;1f");
}

void ansi_goto(int x, int y)
{
  Serial.print("\x1B[");
  Serial.print(y);
  Serial.print(';');
  Serial.print(x);
  Serial.print('f');
}

PROGMEM prog_char ui_about_str_0[]  = "+--- DUAL PWM/DCC CONTROLLER v1.0 ------------------------------------+";
PROGMEM prog_char ui_about_str_1[]  = "|                                                                     |";
PROGMEM prog_char ui_about_str_2[]  = "|  PPPPPP  WW         WW MM       MM    // DDDDDD     CCCCCC  CCCCCC  |";
PROGMEM prog_char ui_about_str_3[]  = "|  PP   PP WW         WW MMMM   MMMM   //  DD   DD   CC      CC       |";
PROGMEM prog_char ui_about_str_4[]  = "|  PP   PP WW    W    WW MM MM MM MM   //  DD    DD CC      CC        |";
PROGMEM prog_char ui_about_str_5[]  = "|  PPPPPP  WW   WWW   WW MM  MMM  MM   //  DD    DD CC      CC        |";
PROGMEM prog_char ui_about_str_6[]  = "|  PP       WW WW WW WW  MM   M   MM  //   DD    DD CC      CC        |";
PROGMEM prog_char ui_about_str_7[]  = "|  PP       WWWW  WWWW   MM       MM  //   DD   DD   CC      CC       |";
PROGMEM prog_char ui_about_str_8[]  = "|  PP        WW    WW    MM       MM //    DDDDDD     CCCCCC  CCCCCC  |";
PROGMEM prog_char ui_about_str_9[]  = "|                                                                     |";
PROGMEM prog_char ui_about_str_10[] = "+------------------------------------ DUAL PWM/DCC CONTROLLER v1.0 ---+";
PROGMEM prog_char ui_about_str_11[] = "(C) 2013 Gerardo García Peña <killabytenow@gmail.com>";
PROGMEM prog_char ui_about_str_12[] = "This program is free software; you can redistribute it and/or modify it";
PROGMEM prog_char ui_about_str_13[] = "under the terms of the GNU General Public License as published by the Free";
PROGMEM prog_char ui_about_str_14[] = "Software Foundation; either version 3 of the License, or (at your option)";
PROGMEM prog_char ui_about_str_15[] = "any later version.";
PROGMEM prog_char ui_about_str_16[] = "This program is distributed in the hope that it will be useful, but WITHOUT";
PROGMEM prog_char ui_about_str_17[] = "ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or";
PROGMEM prog_char ui_about_str_18[] = "FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for";
PROGMEM prog_char ui_about_str_19[] = "more details.";
PROGMEM prog_char ui_about_str_20[] = "You should have received a copy of the GNU General Public License along";
PROGMEM prog_char ui_about_str_21[] = "with this program; if not, write to the Free Software Foundation, Inc., 51";
PROGMEM prog_char ui_about_str_22[] = "Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA";

PROGMEM const char *ui_about_str[] = {
  ui_about_str_0,  ui_about_str_1,  ui_about_str_2,  ui_about_str_3,  ui_about_str_4,
  ui_about_str_5,  ui_about_str_6,  ui_about_str_7,  ui_about_str_8,  ui_about_str_9,
  ui_about_str_10, ui_about_str_11, ui_about_str_12, ui_about_str_13, ui_about_str_14,
  ui_about_str_15, ui_about_str_16, ui_about_str_17, ui_about_str_18, ui_about_str_19,
  ui_about_str_20, ui_about_str_21, ui_about_str_22,
};

///////////////////////////////////////////////////////////////////////////////
// CLI
///////////////////////////////////////////////////////////////////////////////

struct {
  char *token;
  int   value;
} cli_tokens[] = {
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

  if(ntokens == 1) {
      if(!strcasecmp(token[0], "about")) { // [ALL] about
        ui_hello_draw();
      } else 
      if(!strcasecmp(token[0], "menu" )) { // [ALL] menu
        ui_curr = (struct ui_screen *) &ui_hello;
        current_ui_handler = uiHandler;
      } else
      if(!strcasecmp(token[0], "off"  )
      || !strcasecmp(token[0], "pwm"  )
      || !strcasecmp(token[0], "dcc"  )) { // [ALL] off / pwm / dcc
        boosterMngrSelectByName(token[0]);
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


