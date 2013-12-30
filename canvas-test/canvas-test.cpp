#include <gtk/gtk.h>
#include <stdlib.h>
#include "UTFT/UTFT.h"

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

static void close_window(
		GtkWidget *widget,
		gpointer   data)
{
	UTFT *utft = (UTFT *) data;

	if(utft->surface)
		cairo_surface_destroy(utft->surface);

	gtk_main_quit();
}

int main(int argc, char *argv[])
{
	GtkWidget *window;
	GOptionContext *context;
	const gchar *model;
	int zoom = 1;
	GError *error = NULL;
	GOptionEntry entries[] = {
		{ "model", 'm', 0, G_OPTION_ARG_STRING, &model, "Emulated UTFT screen", "M" },
		{ "zoom",  'z', 0, G_OPTION_ARG_INT,    &zoom,  "Zoom factor",          "Z" },
		{ NULL },
	};

	context = g_option_context_new("- UTFT emulator");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));

	if(!gtk_init_with_args(&argc, &argv, "OJETE", entries, NULL, &error))
		exit(1);

	if(!model)
		model = "SSD1963_800";
	if(zoom < 1) {
		g_print("Cannot accept a zoom factor smaller than 1");
		exit(1);
	}
	g_print("model = %s\n", model);
	g_print("zoom  = %d\n", zoom);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Drawing Area");

	g_signal_connect(window, "destroy", G_CALLBACK(close_window), NULL);

	gtk_container_set_border_width(GTK_CONTAINER(window), 8);

g_print("PEROLAIRE\n");
	UTFT utft = UTFT(TFT01_22SP, 0, 0, 0, 0, 0);
	utft.InitLCD(LANDSCAPE);
	GtkWidget *utft_widget = utft.gtk_getLCDWidget();
	gtk_container_add(GTK_CONTAINER(window), utft_widget);
g_print("PEROLAS\n");
	gtk_widget_show(utft_widget);


	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
