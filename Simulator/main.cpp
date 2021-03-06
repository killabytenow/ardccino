/*****************************************************************************
 * main.cpp
 *
 * GTK hw simulator
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

#include <gtk/gtk.h>
#include <pty.h>
#include <vte/vte.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "config.h"
#include "UTFT/UTFT.h"

#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>


#ifdef SIMULATOR

struct models_struct {
	const char  *name;
	int   key;
} models[] = {
	{ "HX8347A",         HX8347A,        },
	{ "ILI9327",         ILI9327,        },
	{ "SSD1289",         SSD1289,        },
	{ "ILI9325C",        ILI9325C,       },
	{ "ILI9325D_8",      ILI9325D_8,     },
	{ "ILI9325D_16",     ILI9325D_16,    },
	{ "ILI9325D_16ALT",  ILI9325D_16ALT, },
	{ "HX8340B_8",       HX8340B_8,      },
	{ "HX8340B_S",       HX8340B_S,      },
	{ "HX8352A",         HX8352A,        },
	{ "ST7735",          ST7735,         },
	{ "ST7735S",         ST7735S,        },
	{ "PCF8833",         PCF8833,        },
	{ "S1D19122",        S1D19122,       },
	{ "SSD1963_480",     SSD1963_480,    },
	{ "SSD1963_800",     SSD1963_800,    },
	{ "SSD1963_800ALT",  SSD1963_800ALT, },
	{ "S6D1121_8",       S6D1121_8,      },
	{ "S6D1121_16",      S6D1121_16,     },
	{ "SSD1289LATCHED",  SSD1289LATCHED, },
	{ "ILI9320_8",       ILI9320_8,      },
	{ "ILI9320_16",      ILI9320_16,     },
	{ "SSD1289_8",       SSD1289_8,      },
	{ "ILI9481",         ILI9481,        },
	{ "S6D0164",         S6D0164,        },
	{ "ILI9341_S5P",     ILI9341_S5P,    },
	{ "ILI9341_S4P",     ILI9341_S4P,    },
	{ "ITDB32",          ITDB32,         },
	{ "ITDB32WC",        ITDB32WC,       },
	{ "TFT01_32W",       TFT01_32W,      },
	{ "ITDB32S",         ITDB32S,        },
	{ "TFT01_32",        TFT01_32,       },
	{ "CTE32",           CTE32,          },
	{ "GEEE32",          GEEE32,         },
	{ "ITDB24",          ITDB24,         },
	{ "ITDB24D",         ITDB24D,        },
	{ "ITDB24DWOT",      ITDB24DWOT,     },
	{ "ITDB28",          ITDB28,         },
	{ "TFT01_24_8",      TFT01_24_8,     },
	{ "TFT01_24_16",     TFT01_24_16,    },
	{ "ITDB22",          ITDB22,         },
	{ "GEEE22",          GEEE22,         },
	{ "ITDB22SP",        ITDB22SP,       },
	{ "ITDB32WD",        ITDB32WD,       },
	{ "TFT01_32WD",      TFT01_32WD,     },
	{ "CTE32W",          CTE32W,         },
	{ "ITDB18SP",        ITDB18SP,       },
	{ "LPH9135",         LPH9135,        },
	{ "ITDB25H",         ITDB25H,        },
	{ "ITDB43",          ITDB43,         },
	{ "ITDB50",          ITDB50,         },
	{ "TFT01_50",        TFT01_50,       },
	{ "CTE50",           CTE50,          },
	{ "ITDB24E_8",       ITDB24E_8,      },
	{ "ITDB24E_16",      ITDB24E_16,     },
	{ "INFINIT32",       INFINIT32,      },
	{ "ELEE32_REVA",     ELEE32_REVA,    },
	{ "GEEE24",          GEEE24,         },
	{ "GEEE28",          GEEE28,         },
	{ "ELEE32_REVB",     ELEE32_REVB,    },
	{ "TFT01_70",        TFT01_70,       },
	{ "CTE70",           CTE70,          },
	{ "CTE32HR",         CTE32HR,        },
	{ "CTE28",           CTE28,          },
	{ "TFT01_28",        TFT01_28,       },
	{ "CTE22",           CTE22,          },
	{ "TFT01_22",        TFT01_22,       },
	{ "TFT01_18SP",      TFT01_18SP,     },
	{ "TFT01_22SP",      TFT01_22SP,     },
	{ "MI0283QT9",       MI0283QT9,      },
};

int model_get_no(const gchar *model)
{
	for(int i = 0;
	    i < (signed) (sizeof(models) / sizeof(struct models_struct));
	    i++)
		if(!strcmp(models[i].name, model))
			return models[i].key;

	return -1;
}

int pty_master, pty_slave;

extern void setup(void);
extern void loop(void);
#ifdef ENABLE_SCREEN
extern UTFT tft;
#endif

static gpointer thread_func(gpointer)
{
	setup();
	while(1)
		loop();
	return NULL;
}

static void close_window(
		GtkWidget *, // widget - unused parameter
		gpointer   data)
{
	g_print("closing window\n");

#ifdef ENABLE_SCREEN
	UTFT *tft = (UTFT *) data;
	if(tft && tft->surface)
		cairo_surface_destroy(tft->surface);
#endif

	gtk_main_quit();
}

void launch_thread(void)
{
	GThread *thread = g_thread_new("MyThread", thread_func, NULL);
	if(!thread) {
		g_print("Error launching thread\n");
		exit(-1);
	}
}

int checktty(struct termios *p, int term_fd)
{
	struct termios ck;
	return (tcgetattr(term_fd, &ck) == 0 &&
		(p->c_lflag == ck.c_lflag) &&
		(p->c_cc[VMIN] == ck.c_cc[VMIN]) &&
		(p->c_cc[VTIME] == ck.c_cc[VMIN]));
}


int main(int argc, char *argv[])
{
	GtkWidget *window;
	GOptionContext *context;
	const gchar *model = NULL;
	int zoom = 1, model_no;
	GError *error = NULL;

	///////////////////////////////////////////////////////////////////////
	// GET COMMAND LINE PARAMETERS
	///////////////////////////////////////////////////////////////////////

	// build cli params
	GOptionEntry entries[] = {
		{ "model", 'm', 0, G_OPTION_ARG_STRING, &model, "Emulated UTFT screen", "M"  },
		{ "zoom",  'z', 0, G_OPTION_ARG_INT,    &zoom,  "Zoom factor",          "Z"  },
		{ NULL,      0, 0, G_OPTION_ARG_NONE,   NULL,   NULL,                   NULL },
	};
	context = g_option_context_new("UTFT emulator");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));

	// init app fetching app params
	if(!gtk_init_with_args(&argc, &argv, "OJETE", entries, NULL, &error))
		exit(1);

	// set default values (if params were not set)
	if(!model || !strlen(model))
		//model = "SSD1963_800";
		model = "TFT01_22SP";
	if((model_no = model_get_no(model)) < 0) {
		g_print("Unknown screen model '%s'.\n", model);
		exit(1);
	} else {
		tft = UTFT(model_no, 0, 0, 0, 0, 0);
	}
	if(zoom < 1) {
		g_print("Cannot accept a zoom factor smaller than 1");
		exit(1);
	} else {
		//tft.zoom = zoom;
	}

	// print debug crap
	g_print("Assigned parameters\n");
	g_print("\tmodel = %s (%d)\n", model, model_no);
	g_print("\tzoom  = %d\n", zoom);

	///////////////////////////////////////////////////////////////////////
	// BUILD THE MAIN APP WINDOW
	///////////////////////////////////////////////////////////////////////

	// build the UTFT widget (screen)
#ifdef ENABLE_SCREEN
	tft.InitLCD(LANDSCAPE);
	g_print("Screen size %dx%d\n", tft.getDisplayXSize(), tft.getDisplayYSize());
#endif

	// create the terminal window
	GtkWidget *vte = vte_terminal_new();
	vte_terminal_set_background_transparent(VTE_TERMINAL(vte), FALSE);
	vte_terminal_set_size(VTE_TERMINAL(vte), 80, 45);
	vte_terminal_set_scrollback_lines(VTE_TERMINAL(vte), -1); /* infinite scrollback */
	vte_terminal_set_emulation(VTE_TERMINAL(vte), "xterm");
	if(openpty(&pty_master, &pty_slave, NULL, NULL, NULL) < 0) {
		perror("openpty error");
		exit(-1);
	}


	struct termios attr;
	struct termios newterm;
	errno=0;
	tcgetattr(pty_master, &attr);  /* get current stty settings*/

	newterm = attr; 
	newterm.c_lflag &= ~(ECHO | ICANON); 
	newterm.c_cc[VMIN] = 0; 
	newterm.c_cc[VTIME] = 0; 

	if(tcgetattr(pty_master, &attr) != 0
	|| tcsetattr(pty_master, TCSAFLUSH, &newterm) != 0
	|| checktty(&newterm, pty_master) == 0)
	{
		perror("cosa");
		exit(1);
	}

	Serial.set_fd(pty_slave);
	VtePty *vte_pty = vte_pty_new_foreign(pty_master, &error);
	vte_terminal_set_pty_object(VTE_TERMINAL(vte), vte_pty);

	vte_terminal_set_scroll_on_keystroke(VTE_TERMINAL (vte), TRUE);

	GtkWidget *vte_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (vte_scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(vte_scrolled_window), vte);

#ifdef ENABLE_SCREEN
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_pack_start(GTK_BOX(box), tft.gtk_getLCDWidget(), FALSE, FALSE, 2);
	gtk_box_pack_end(GTK_BOX(box), vte_scrolled_window, TRUE, TRUE, 2);
#endif

	// create window and attach UTFT screen
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Ardccino Simulator");
	gtk_container_set_border_width(GTK_CONTAINER(window), 8);

#ifdef ENABLE_SCREEN
	gtk_container_add(GTK_CONTAINER(window), box);
#else
	gtk_container_add(GTK_CONTAINER(window), vte_scrolled_window);
#endif

	// connect signals
#ifdef ENABLE_SCREEN
	g_signal_connect(window, "destroy", G_CALLBACK(close_window), &tft);
#endif

	// here we go
	gtk_widget_show_all(window);

	/* Create new thread */
	launch_thread();

	gtk_main();

	return 0;
}

#endif
