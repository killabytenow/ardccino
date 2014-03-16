#include <gtk/gtk.h>
#include "config.h"
#include "UTFT/UTFT.h"

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
};

extern void setup(void);
extern void loop(void);

static gpointer thread_func(gpointer data)
{
	//setup();
	while(1) {
		gdk_threads_enter();
		//loop();
		sleep(1);
		gdk_threads_leave();
	}

	return( NULL );
}

static void close_window(
		GtkWidget *widget,
		gpointer   data)
{
	UTFT *utft = (UTFT *) data;

	g_print("closing window\n");
	if(utft && utft->surface) {
		g_print("- destroy surface\n");
		cairo_surface_destroy(utft->surface);
	}

	gtk_main_quit();
}

int main(int argc, char *argv[])
{
	GtkWidget *window;
	GOptionContext *context;
	const gchar *model;
	int zoom = 1;
	GError *error = NULL;
	GThread   *thread;

	if(!g_thread_supported())
		g_thread_init(NULL);
	gdk_threads_init();

	///////////////////////////////////////////////////////////////////////
	// GET COMMAND LINE PARAMETERS
	///////////////////////////////////////////////////////////////////////

	// build cli params
	GOptionEntry entries[] = {
		{ "model", 'm', 0, G_OPTION_ARG_STRING, &model, "Emulated UTFT screen", "M" },
		{ "zoom",  'z', 0, G_OPTION_ARG_INT,    &zoom,  "Zoom factor",          "Z" },
		{ NULL },
	};
	context = g_option_context_new("UTFT emulator");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));

	// init app fetching app params
	gdk_threads_enter();

	if(!gtk_init_with_args(&argc, &argv, "OJETE", entries, NULL, &error))
		exit(1);

	// set default values (if params were not set)
	if(!model)
		model = "SSD1963_800";
	if(zoom < 1) {
		g_print("Cannot accept a zoom factor smaller than 1");
		exit(1);
	}

	// print debug crap
	g_print("Assigned parameters\n");
	g_print("\tmodel = %s\n", model);
	g_print("\tzoom  = %d\n", zoom);

	///////////////////////////////////////////////////////////////////////
	// BUILD THE MAIN APP WINDOW
	///////////////////////////////////////////////////////////////////////

	// build the UTFT widget (screen)
	UTFT utft = UTFT(TFT01_22SP, 0, 0, 0, 0, 0);
	utft.zoom = zoom;
	utft.InitLCD(LANDSCAPE);

	// create window and attach UTFT screen
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Drawing Area");
	gtk_container_set_border_width(GTK_CONTAINER(window), 8);

	gtk_container_add(GTK_CONTAINER(window), utft.gtk_getLCDWidget());
	//gtk_window_set_resizable (GTK_WINDOW(mainWindow), FALSE);

	//gtk_window_set_default_size(GTK_WINDOW(mainWindow), utft.getDisplayXSize(), utft.getDisplayYSize());
	g_print("Screen x = %d y = %d\n", utft.getDisplayXSize(), utft.getDisplayYSize());
	//gtk_window_set_resizable (GTK_WINDOW(mainWindow), FALSE);

	// connect signals
	g_signal_connect(window, "destroy", G_CALLBACK(close_window), &utft);

	// here we go
	gtk_widget_show(utft.gtk_getLCDWidget());
	gtk_widget_show_all(window);

	/* Create new thread */
	thread = g_thread_create(thread_func, NULL, FALSE, &error);
	if(!thread) {
		g_print( "Error: %s\n", error->message );
		return( -1 );
	}

//utft.drawCircle(20, 20, 10);
	gtk_main();
	//while (gtk_main_iteration()) { ; }

	gdk_threads_leave();

	return 0;
}

#endif
