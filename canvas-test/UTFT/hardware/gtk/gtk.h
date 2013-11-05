#include <stdlib.h>

/* Surface to store current scribbles */

/* Create a new surface of the appropriate size to store our scribbles */
static gboolean configure_event_cb(
	GtkWidget         *widget,
	GdkEventConfigure *event,
	gpointer           data)
{
	UTFT *utft = (UTFT *) data;

	g_print("configure_event_cb: received\n");

	if(utft->surface)
		cairo_surface_destroy(utft->surface);

	utft->surface =
		gdk_window_create_similar_surface(
			gtk_widget_get_window(widget),
			CAIRO_CONTENT_COLOR,
			gtk_widget_get_allocated_width(widget),
			gtk_widget_get_allocated_height(widget));

	/* Initialize the surface to white */
	utft->gtk_clear_surface();

	/* We've handled the configure event, no need for further processing. */
	return TRUE;
}

/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static gboolean draw_cb(
		GtkWidget *widget,
		cairo_t   *cr,
		gpointer   data)
{
	UTFT *utft = (UTFT *) data;

	cairo_set_source_surface(cr, utft->surface, 0, 0);
	cairo_paint(cr);

	return FALSE;
}

/* Draw a rectangle on the surface at the given position */
void UTFT::__set_pixel(void)
{
	cairo_t *cr;

	/* Paint to the surface, where we store our state */
	cr = cairo_create(surface);

	cairo_rectangle(
		cr,
		gtk_last_x * zoom, gtk_last_y * zoom,
		zoom, zoom);
	// word color = ((r&248)<<8 | (g&252)<<3 | (b&248)>>3);
	double r = ((double) ((((unsigned) gtk_color) >> 8) & 0xff)) / 247.0;
	double g = ((double) ((((unsigned) gtk_color) >> 3) & 0xff)) / 251.0;
	double b = ((double) ((((unsigned) gtk_color) << 3) & 0xff)) / 247.0;
	g_print("Using color (%.2f, %.2f, %.2f)", r, g, b);
	cairo_set_source_rgb(cr, r , g, b);
	cairo_fill(cr);

	cairo_destroy(cr);

	/* Now invalidate the affected region of the drawing area. */
	gtk_widget_queue_draw_area(
		drawing_area,
		gtk_last_x * zoom, gtk_last_y * zoom,
		zoom, zoom);
}

/* Handle button press events by either drawing a rectangle
 * or clearing the surface, depending on which button was pressed.
 * The ::button-press signal handler receives a GdkEventButton
 * struct which contains this information.
 */
#if 0
static gboolean button_press_event_cb(
		GtkWidget      *widget,
		GdkEventButton *event,
		gpointer        data)
{
	UTFT *utft = (UTFT *) data;

	/* paranoia check, in case we haven't gotten a configure event */
	if(utft->surface == NULL)
		return FALSE;

	if(event->button == 1) {
		utft->__set_pixel();
	} else if (event->button == 3) {
		utft->gtk_clear_surface();
		gtk_widget_queue_draw(widget);
	}

	/* We've handled the event, stop processing */
	return TRUE;
}

/* Handle motion events by continuing to draw if button 1 is
 * still held down. The ::motion-notify signal handler receives
 * a GdkEventMotion struct which contains this information.
 */
static gboolean motion_notify_event_cb(
				GtkWidget      *widget,
				GdkEventMotion *event,
				gpointer        data)
{
	int x, y;
	GdkModifierType state;

	/* paranoia check, in case we haven't gotten a configure event */
	if(surface == NULL)
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
	gdk_window_get_pointer(event->window, &x, &y, &state);

	if(state & GDK_BUTTON1_MASK)
		utft->__set_pixel();

	/* We've handled it, stop processing */
	return TRUE;
}
#endif

void UTFT::_hw_special_init()
{
	// initialize GTK LCD state attributes
	gtk_last_x = gtk_last_y = 0;
	gtk_color = 0;
	surface = NULL;

	// create GTK surface and assign event handlers
	fixed = gtk_fixed_new();
	//gtk_container_add(GTK_CONTAINER(window), fixed);
	gtk_widget_show(fixed);

	drawing_area = gtk_drawing_area_new();
	/* set a minimum size */
	gtk_widget_set_size_request(drawing_area, disp_x_size * zoom, disp_y_size * zoom);

	gtk_fixed_put(GTK_FIXED(fixed), drawing_area, 0, 0);

	/* Signals used to handle the backing surface */
	g_signal_connect(drawing_area, "draw",
				G_CALLBACK(draw_cb), this);
	g_signal_connect(drawing_area, "configure-event",
				G_CALLBACK(configure_event_cb), this);

	/* Event signals */
#if 0
	g_signal_connect(drawing_area, "motion-notify-event",
				G_CALLBACK(motion_notify_event_cb), this);
	g_signal_connect(drawing_area, "button-press-event",
				G_CALLBACK(button_press_event_cb), this);
#endif

	/* Ask to receive events the drawing area doesn't normally
	 * subscribe to. In particular, we need to ask for the
	 * button press and motion notify events that want to handle.
	 */
	gtk_widget_set_events(drawing_area,
				gtk_widget_get_events(drawing_area)
				| GDK_BUTTON_PRESS_MASK
				| GDK_POINTER_MOTION_MASK
				| GDK_POINTER_MOTION_HINT_MASK);
}

GtkWidget *UTFT::gtk_getLCDWidget(void)
{
	if(!fixed) {
		g_print("Hardware not initialized! Aborting!\n");
		abort();
	}
	return fixed;
}

void UTFT::gtk_clear_surface(void)
{
	cairo_t *cr;

	cr = cairo_create(surface);

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	cairo_destroy(cr);
}

void UTFT::LCD_Writ_Bus(char VH,char VL, byte mode)
{   
	g_print("LCD_Writ_Bus() executed. Aborting.");
	abort();
}

void UTFT::_set_direction_registers(byte mode)
{
}

void UTFT::_fast_fill_16(int ch, int cl, long pix)
{
	// ch - color high
	// cl - color low
	// pix - number of pixels X*Y
}

void UTFT::_fast_fill_8(int ch, long pix)
{
	long blocks;

    	// ch - color high?
	// pix - number of pixels X*Y
}

void UTFT::_convert_float(char *buf, double num, int width, byte prec)
{
	char format[64];
	
	sprintf(format, "%%%i.%if", width, prec);
	sprintf(buf, format, num);
}
