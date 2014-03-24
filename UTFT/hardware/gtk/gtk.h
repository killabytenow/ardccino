#include <stdlib.h>

#define COLOR_W(ch, cl)	\
			 ((double) (((unsigned) (ch)) & 0xf8)) / 255.0,         \
			 ((double) (                                            \
				     (((unsigned) (ch & 0x07)) << 5)            \
				   | (((unsigned) (cl & 0xe0)) >> 3))) / 255.0, \
			 ((double) (((unsigned) (cl)) & 0x1f)) / 255.0
#define COLOR_W_R(ch, cl)	(((unsigned char) ch) & 0xf8)
#define COLOR_W_G(ch, cl)	(((((unsigned char) ch) & 0x07) << 5) \
				| ((((unsigned char) cl) & 0xe0) >> 3))
#define COLOR_W_B(ch, cl)	(((unsigned char) cl) & 0x1f)

static gboolean do_configure_event(
       GtkWidget         *widget,
       GdkEventConfigure *event,
       gpointer           data)
{
	UTFT *utft = (UTFT *) data;

	g_print(__FILE__ ":%s: received\n", __func__);
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

	g_print(__FILE__ ":%s: out\n", __func__);
	return TRUE;
}

static gboolean do_draw_event(
	GtkWidget      *widget, 
	cairo_t        *cr,
	gpointer        data)
{
	UTFT *utft = (UTFT *) data;

	g_print(__FILE__ ":%s: received\n", __func__);
	if(utft->lcd_buffer == NULL)
		return FALSE;

	// copy lcd buffer
	gdk_cairo_set_source_pixbuf(cr, utft->lcd_buffer, 0, 0);
	cairo_paint(cr);

	g_print(__FILE__ ":%s: out\n", __func__);

	return TRUE;
}


/* Handle button press events by either drawing a rectangle
 * or clearing the surface, depending on which button was pressed.
 * The ::button-press signal handler receives a GdkEventButton
 * struct which contains this information.
 */
static gboolean button_press_event_cb(
		GtkWidget      *widget,
		GdkEventButton *event,
		gpointer        data)
{
	UTFT *utft = (UTFT *) data;

	int x = event->x;
	int y = event->y;

	g_print(__FILE__ ":%s: event (%.0f, %.0f) -> (%d, %d) pixbuf(%d, %d) orient=%d\n",
			__func__,
			event->x, event->y,
			x, y,
			gdk_pixbuf_get_width(utft->lcd_buffer),
			gdk_pixbuf_get_height(utft->lcd_buffer),
			utft->orient);

	// paranoia check, in case we haven't gotten a configure event
	if(utft->surface == NULL) {
		g_print(__FILE__ ":%s: utft->surface is null\n", __func__);
		return FALSE;
	}
	if(event->button == 1) {
		utft->drawCircle(x, y, 10);
	} else if (event->button == 3) {
		utft->gtk_clear_surface();
	}
	gtk_widget_queue_draw(widget);

	// We've handled the event, stop processing
	g_print(__FILE__ ":%s: out\n", __func__);
	return TRUE;
}

void UTFT::_hw_special_init()
{
	int w, h;

	w = disp_x_size + 1;
	h = disp_y_size + 1;
	if(orient == LANDSCAPE)
		swap(int, w, h);
	g_print("draw surface size %dx%d\n", w, h);

	// initialize GTK LCD state attributes
	gtk_last_x = gtk_last_y   = 0;
	gtk_area_x1 = gtk_area_x2 = 0;
	gtk_area_y1 = gtk_area_y2 = 0;
	surface = NULL;

	// create GTK surface and assign event handlers
	drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(drawing_area, w, h);

	//fixed = gtk_fixed_new();
	//gtk_fixed_put(GTK_FIXED(fixed), drawing_area, 0, 0);

	// create the LCD buffer (it is where the UTFT code will write
	// directly using the LCD bus primitives)
	lcd_buffer = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, w, h);

	// Signals used to handle the backing surface
	g_signal_connect(drawing_area, "draw",               G_CALLBACK(do_draw_event),         this);
	g_signal_connect(drawing_area, "configure-event",    G_CALLBACK(do_configure_event),    this);
	g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(button_press_event_cb), this);

	align = gtk_alignment_new(0.5, 0.0, 0.0, 0.0);
	gtk_container_add(GTK_CONTAINER(align), drawing_area); 

	// Ask to receive events the drawing area doesn't normally
	// subscribe to. In particular, we need to ask for the
	// button press and motion notify events that want to handle.
	gtk_widget_set_events(drawing_area,
				gtk_widget_get_events(drawing_area)
				| GDK_BUTTON_PRESS_MASK);
				//| GDK_POINTER_MOTION_MASK
				//| GDK_POINTER_MOTION_HINT_MASK);

	gtk_widget_show(drawing_area);
	gtk_widget_show(align);

	g_print(__FILE__ ":%s: finished\n", __func__);
}

GtkWidget *UTFT::gtk_getLCDWidget(void)
{
	if(!align) {
		g_print(__FILE__ ":%s: Hardware not initialized! Aborting!\n", __func__);
		abort();
	}
	return align;
}

void UTFT::gtk_clear_surface(void)
{
	cairo_t *cr;

	cr = cairo_create(surface);

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	cairo_destroy(cr);

	gtk_widget_queue_draw_area(drawing_area, 0, 0, disp_x_size + 1, disp_y_size + 1);
}

void UTFT::LCD_Write_COM(char VL)  
{   
	g_print("LCD_Write_COM() executed. Aborting.");
	abort();
}

//void UTFT::refresh(void)
//{
//	cairo_t *cr;
//
//	/* Paint to the surface, where we store our state */
//	cr = cairo_create(surface);
//	cairo_rectangle(cr, zoom * gtk_last_x, zoom * gtk_last_y, zoom, zoom);
//
//	//g_print("Using color (%.2f, %.2f, %.2f)\n", COLOR_W(gtk_color));
//	cairo_set_source_rgb(cr, COLOR_W(VH, VL));
//	cairo_fill(cr);
//	cairo_stroke(cr);
//
//	cairo_destroy(cr);
//
//}

void UTFT::LCD_Write_DATA(char VH, char VL)
{
	int width, height, rowstride;
	guchar *pixels, *p;

	g_print(__FILE__ ":%s: pixbuf(%d, %d) last(%d, %d)\n",
			__func__,
			gdk_pixbuf_get_width(lcd_buffer),
			gdk_pixbuf_get_height(lcd_buffer),
			gtk_last_x, gtk_last_y);

	g_assert(gdk_pixbuf_get_n_channels(lcd_buffer) == 3);
	g_assert(gtk_area_x1 <= gtk_last_x && gtk_last_x <= gtk_area_x2
	      && gtk_area_y1 <= gtk_last_y && gtk_last_y <= gtk_area_y2);
	g_assert(gtk_last_x >= 0 && gtk_last_x < gdk_pixbuf_get_width(lcd_buffer));
	g_assert(gtk_last_y >= 0 && gtk_last_y < gdk_pixbuf_get_height(lcd_buffer));

	rowstride = gdk_pixbuf_get_rowstride(lcd_buffer);

	pixels = gdk_pixbuf_get_pixels(lcd_buffer);
	p = pixels + (gtk_last_y * rowstride) + (gtk_last_x * 3);

	p[0] = COLOR_W_R(VH, VL);
	p[1] = COLOR_W_G(VH, VL);
	p[2] = COLOR_W_B(VH, VL);

	if(++gtk_last_x > gtk_area_x2) {
		gtk_last_x = gtk_area_x1;
		gtk_last_y++;
	}

	// Now invalidate the affected region of the drawing area
	gtk_widget_queue_draw_area(drawing_area, gtk_last_x, gtk_last_y, 1, 1);
}

void UTFT::LCD_Write_DATA(char VL)
{
	g_print("LCD_Write_DATA(8) executed. Aborting.");
	abort();
}

void UTFT::LCD_Writ_Bus(char VH,char VL, byte mode)
{   
	g_print(__FILE__ ":%s: Aborting!\n", __func__);
	abort();
}

void UTFT::_set_direction_registers(byte mode)
{
}

// ch - color high
// cl - color low
// pix - number of pixels X*Y
void UTFT::_fast_fill_16(int ch, int cl, long pix)
{
	int rowstride;
	guchar *p;

	g_assert(((gtk_area_x2 - gtk_area_x1 + 1) * (gtk_area_y2 - gtk_area_y1 + 1)) == pix);
	g_assert(gtk_last_x == gtk_area_x1);
	g_assert(gtk_last_y == gtk_area_y1);

	g_print(__FILE__ ":%s: called\n", __func__);
	rowstride = gdk_pixbuf_get_rowstride(lcd_buffer);
	guchar *pixels = gdk_pixbuf_get_pixels(lcd_buffer);
	byte r = ((unsigned char) ch) & 0xf8;
	byte g = ((((unsigned char) ch) & 0x07) << 5) | ((((unsigned char) cl) & 0xe0) >> 3);
	byte b = ((unsigned char) cl) & 0x1f;
	for(; gtk_last_y < gtk_area_y2; gtk_last_y++) {
		p = pixels + (gtk_last_y * rowstride) + (gtk_last_x * 3);
		for(gtk_last_x = gtk_area_x1; gtk_last_x < gtk_area_x2; gtk_last_x++) {
			*p++ = r;
			*p++ = g;
			*p++ = b;
		}
	}
		
	/* Now invalidate the affected region of the drawing area. */
	gtk_widget_queue_draw_area(
		drawing_area,
		gtk_area_x1,
		gtk_area_y1,
		gtk_area_x2 - gtk_area_x1 + 1,
		gtk_area_y2 - gtk_area_y1 + 1);
}

void UTFT::_fast_fill_8(int ch, long pix)
{
    	// ch - color high?
	// pix - number of pixels X*Y
}

void UTFT::_convert_float(char *buf, double num, int width, byte prec)
{
	char format[64];
	
	sprintf(format, "%%%i.%if", width, prec);
	sprintf(buf, format, num);
}
