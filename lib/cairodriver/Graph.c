#include "cairodriver.h"
#include <cairo-ps.h>
#include <cairo-pdf.h>
#include <cairo-svg.h>
#if defined(USE_X11) && CAIRO_HAS_XLIB_SURFACE
#include <cairo-xlib.h>
#endif

#include <unistd.h>
#ifndef __MINGW32__
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

#if defined(USE_X11) && CAIRO_HAS_XLIB_SURFACE
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

/* globals */
char *file_name;
int file_type;
int is_vector;
int width, height, stride;
unsigned char *grid;
int modified;
int auto_write;
int mapped;

/* background color */
double bgcolor_r, bgcolor_g, bgcolor_b, bgcolor_a;

/* cairo objects */
cairo_surface_t *surface;
cairo_t *cairo;

static void init_cairo(void);
static int ends_with(const char *string, const char *suffix);
static void map_file(void);

#if defined(USE_X11) && CAIRO_HAS_XLIB_SURFACE
static int init_xlib(void)
{
    Display *dpy;
    Drawable win;
    unsigned long xid;
    XVisualInfo templ;
    XVisualInfo *vinfo;
    int count;
    Window root;
    unsigned int depth;
    int si;
    unsigned int ui;
    Visual *visual;
    char *p;

    p = getenv("GRASS_CAIRO_DRAWABLE");
    if (!p || sscanf(p, "%li", &xid) != 1)
	G_fatal_error("invalid Drawable XID: %s", p);
    win = xid;

    dpy = XOpenDisplay(NULL);
    if (!dpy)
	G_fatal_error("Unable to open display");

    p = getenv("GRASS_CAIRO_VISUAL");
    if (!p || sscanf(p, "%li", &xid) != 1)
	G_fatal_error("invalid Visual XID: %s", p);
    templ.visualid = xid;

    vinfo = XGetVisualInfo(dpy, VisualIDMask, &templ, &count);
    if (!vinfo || !count)
	G_fatal_error("Unable to obtain visual");
    visual = vinfo[0].visual;

    if (!XGetGeometry
	(dpy, win, &root, &si, &si, &width, &height, &ui, &depth))
	G_fatal_error("Unable to query drawable");

    surface = cairo_xlib_surface_create(dpy, win, visual, width, height);

    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
	G_fatal_error("Failed to initialize Xlib surface");

    cairo = cairo_create(surface);

    file_name = "<X11>";
    file_type = FTYPE_X11;

    screen_right = screen_left + width;
    screen_bottom = screen_top + height;

    return 0;
}
#endif

static int init_file(void)
{
    int do_read = 0;
    int do_map = 0;
    char *p;

    /* set image properties */
    width = screen_right - screen_left;
    height = screen_bottom - screen_top;
    stride = width * 4;

    /* get file name */
    p = getenv("GRASS_CAIROFILE");
    if (!p || strlen(p) == 0)
	p = DEFAULT_FILE_NAME;

    file_name = p;

    /* get file type (from extension) */
    if (file_type == FTYPE_X11) ;	/* skip */
    else if (ends_with(file_name, ".ppm"))
	file_type = FTYPE_PPM;
    else if (ends_with(file_name, ".bmp"))
	file_type = FTYPE_BMP;
#if CAIRO_HAS_PNG_FUNCTIONS
    else if (ends_with(file_name, ".png"))
	file_type = FTYPE_PNG;
#endif
#if CAIRO_HAS_PDF_SURFACE
    else if (ends_with(file_name, ".pdf"))
	file_type = FTYPE_PDF;
#endif
#if CAIRO_HAS_PS_SURFACE
    else if (ends_with(file_name, ".ps"))
	file_type = FTYPE_PS;
#endif
#if CAIRO_HAS_SVG_SURFACE
    else if (ends_with(file_name, ".svg"))
	file_type = FTYPE_SVG;
#endif
    else
	G_fatal_error("Unknown file extension: %s", p);
    G_debug(1, "File type: %s (%d)", file_name, file_type);

    switch (file_type) {
    case FTYPE_PDF:
    case FTYPE_PS:
    case FTYPE_SVG:
	is_vector = 1;
	break;
    }

    p = getenv("GRASS_CAIRO_MAPPED");
    do_map = p && strcmp(p, "TRUE") == 0 && ends_with(file_name, ".bmp");

    p = getenv("GRASS_CAIRO_READ");
    do_read = p && strcmp(p, "TRUE") == 0;

    if (is_vector) {
	do_read = do_map = 0;
	bgcolor_a = 1.0;
    }

    if (do_read && access(file_name, 0) != 0)
	do_read = 0;

    G_message
	("cairo: collecting to file: %s,\n     GRASS_WIDTH=%d, GRASS_HEIGHT=%d",
	 file_name, width, height);

    if (do_read && do_map)
	map_file();

    if (!mapped && !is_vector)
	grid = G_malloc(height * stride);

    init_cairo();

    if (!do_read && !is_vector) {
	Cairo_Erase();
	modified = 1;
    }

    if (do_read && !mapped)
	read_image();

    if (do_map && !mapped) {
	write_image();
	map_file();
	init_cairo();
    }

    return 0;
}

int Cairo_Graph_set(int argc, char **argv)
{
    char *p;

    G_gisinit("Cairo driver");
    G_debug(1, "Cairo_Graph_set");

    /* get background color */
    p = getenv("GRASS_BACKGROUNDCOLOR");
    if (p && *p) {
	unsigned int red, green, blue;

	if (sscanf(p, "%02x%02x%02x", &red, &green, &blue) == 3) {
	    bgcolor_r = CAIROCOLOR(red);
	    bgcolor_g = CAIROCOLOR(green);
	    bgcolor_b = CAIROCOLOR(blue);
	}
	else
	    G_fatal_error("Unknown background color: %s", p);
    }
    else
	bgcolor_r = bgcolor_g = bgcolor_b = 1.0;

    /* get background transparency setting */
    p = getenv("GRASS_TRANSPARENT");
    if (p && strcmp(p, "TRUE") == 0)
	bgcolor_a = 0.0;
    else
	bgcolor_a = 1.0;

    p = getenv("GRASS_AUTO_WRITE");
    auto_write = p && strcmp(p, "TRUE") == 0;

#if defined(USE_X11) && CAIRO_HAS_XLIB_SURFACE
    p = getenv("GRASS_CAIRO_DRAWABLE");
    if (p)
	return init_xlib();
#endif
    return init_file();
}

void Cairo_Graph_close(void)
{
    G_debug(1, "Cairo_Graph_close");

    write_image();

    if (cairo) {
	cairo_destroy(cairo);
	cairo = NULL;
    }
    if (surface) {
	cairo_surface_destroy(surface);
	surface = NULL;
    }
}

static void init_cairo(void)
{
    G_debug(1, "init_cairo");

    /* create cairo surface */
    switch (file_type) {
    case FTYPE_PPM:
    case FTYPE_BMP:
    case FTYPE_PNG:
	surface =
	    (cairo_surface_t *) cairo_image_surface_create_for_data(grid,
								    CAIRO_FORMAT_ARGB32,
								    width,
								    height,
								    stride);
	break;
#if CAIRO_HAS_PDF_SURFACE
    case FTYPE_PDF:
	surface =
	    (cairo_surface_t *) cairo_pdf_surface_create(file_name,
							 (double)width,
							 (double)height);
	break;
#endif
#if CAIRO_HAS_PS_SURFACE
    case FTYPE_PS:
	surface =
	    (cairo_surface_t *) cairo_ps_surface_create(file_name,
							(double)width,
							(double)height);
	break;
#endif
#if CAIRO_HAS_SVG_SURFACE
    case FTYPE_SVG:
	surface =
	    (cairo_surface_t *) cairo_svg_surface_create(file_name,
							 (double)width,
							 (double)height);
	break;
#endif
    default:
	G_fatal_error("Unknown Cairo surface type");
	break;
    }

    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
	G_fatal_error("Failed to initialize Cairo surface");

    cairo = cairo_create(surface);
}

/* Returns TRUE if string ends with suffix (case insensitive) */
static int ends_with(const char *string, const char *suffix)
{
    if (strlen(string) < strlen(suffix))
	return FALSE;

    return G_strcasecmp(suffix,
			string + strlen(string) - strlen(suffix)) == 0;
}

static void map_file(void)
{
#ifndef __MINGW32__
    size_t size = HEADER_SIZE + width * height * sizeof(unsigned int);
    void *ptr;
    int fd;

    fd = open(file_name, O_RDWR);
    if (fd < 0)
	return;

    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t) 0);
    if (ptr == MAP_FAILED)
	return;

    if (grid) {
	cairo_destroy(cairo);
	cairo_surface_destroy(surface);
	G_free(grid);
    }
    grid = (char *)ptr + HEADER_SIZE;

    close(fd);

    mapped = 1;
#endif
}
