#include <gtk/gtk.h>
#include <stdlib.h>
#include "UTFT/UTFT.h"

/* Surface to store current scribbles */
static cairo_surface_t *surface = NULL;

static void clear_surface(void)
{
	cairo_t *cr;

	cr = cairo_create(surface);

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	cairo_destroy(cr);
}

/* Create a new surface of the appropriate size to store our scribbles */
static gboolean configure_event_cb(
	GtkWidget         *widget,
	GdkEventConfigure *event,
	gpointer           data)
{
	if (surface)
		cairo_surface_destroy (surface);

	surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
				       CAIRO_CONTENT_COLOR,
				       gtk_widget_get_allocated_width (widget),
				       gtk_widget_get_allocated_height (widget));

	/* Initialize the surface to white */
	clear_surface ();

	/* We've handled the configure event, no need for further processing. */
	return TRUE;
}

/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static gboolean
draw_cb (GtkWidget *widget,
 cairo_t   *cr,
 gpointer   data)
{
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

/* Draw a rectangle on the surface at the given position */
static void
draw_brush (GtkWidget *widget,
    gdouble    x,
    gdouble    y)
{
  cairo_t *cr;

  /* Paint to the surface, where we store our state */
  cr = cairo_create (surface);

  cairo_rectangle (cr, x - 3, y - 3, 6, 6);
  cairo_fill (cr);

  cairo_destroy (cr);

  /* Now invalidate the affected region of the drawing area. */
  gtk_widget_queue_draw_area (widget, x - 3, y - 3, 6, 6);
}

/* Handle button press events by either drawing a rectangle
 * or clearing the surface, depending on which button was pressed.
 * The ::button-press signal handler receives a GdkEventButton
 * struct which contains this information.
 */
static gboolean
button_press_event_cb (GtkWidget      *widget,
               GdkEventButton *event,
               gpointer        data)
{
  /* paranoia check, in case we haven't gotten a configure event */
  if (surface == NULL)
    return FALSE;

  if (event->button == 1)
    {
      draw_brush (widget, event->x, event->y);
    }
  else if (event->button == 3)
    {
      clear_surface ();
      gtk_widget_queue_draw (widget);
    }

  /* We've handled the event, stop processing */
  return TRUE;
}

/* Handle motion events by continuing to draw if button 1 is
 * still held down. The ::motion-notify signal handler receives
 * a GdkEventMotion struct which contains this information.
 */
static gboolean
motion_notify_event_cb (GtkWidget      *widget,
                GdkEventMotion *event,
                gpointer        data)
{
	int x, y;
	GdkModifierType state;

	/* paranoia check, in case we haven't gotten a configure event */
	if (surface == NULL)
	return FALSE;

	/* This call is very important; it requests the next motion event.
	* If you don't call gdk_window_get_pointer() you'll only get
	* a single motion event. The reason is that we specified
	* GDK_POINTER_MOTION_HINT_MASK to gtk_widget_set_events().
	* If we hadn't specified that, we could just use event->x, event->y
	* as the pointer location. But we'd also get deluged in events.
	* By requesting the next event as we handle the current one,
	* we avoid getting a huge number of events faster than we
	* can cope.
	*/
	gdk_window_get_pointer (event->window, &x, &y, &state);

	if (state & GDK_BUTTON1_MASK)
		draw_brush(widget, x, y);

	/* We've handled it, stop processing */
	return TRUE;
}

static void
close_window (void)
{
  if (surface)
    cairo_surface_destroy (surface);

  gtk_main_quit ();
}

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

int main(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *frame;
	GtkWidget *da;
	GOptionContext *context;
	const gchar *model;
	GError *error = NULL;
	GOptionEntry entries[] = {
		{ "model", 'm', 0, G_OPTION_ARG_STRING, &model, "Emulated UTFT screen", "M" },
		{ NULL },
	};

	context = g_option_context_new ("- UTFT emulator");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));

	if(!gtk_init_with_args(&argc, &argv, "OJETE", entries, NULL, &error))
		exit(1);

	if(!model)
		model = "SSD1963_800";
	g_print("model = %s\n", model);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Drawing Area");

	g_signal_connect (window, "destroy", G_CALLBACK (close_window), NULL);

	gtk_container_set_border_width (GTK_CONTAINER (window), 8);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (window), frame);

	da = gtk_drawing_area_new ();
	/* set a minimum size */
	gtk_widget_set_size_request (da, 100, 100);

	gtk_container_add (GTK_CONTAINER (frame), da);

	/* Signals used to handle the backing surface */
	g_signal_connect (da, "draw",
	    G_CALLBACK (draw_cb), NULL);
	g_signal_connect (da,"configure-event",
	    G_CALLBACK (configure_event_cb), NULL);

	/* Event signals */
	g_signal_connect (da, "motion-notify-event",
	    G_CALLBACK (motion_notify_event_cb), NULL);
	g_signal_connect (da, "button-press-event",
	    G_CALLBACK (button_press_event_cb), NULL);

	/* Ask to receive events the drawing area doesn't normally
	* subscribe to. In particular, we need to ask for the
	* button press and motion notify events that want to handle.
	*/
	gtk_widget_set_events (da, gtk_widget_get_events (da)
		     | GDK_BUTTON_PRESS_MASK
		     | GDK_POINTER_MOTION_MASK
		     | GDK_POINTER_MOTION_HINT_MASK);

	gtk_widget_show_all (window);

	gtk_main ();

	return 0;
}
