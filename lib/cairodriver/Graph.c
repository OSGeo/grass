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

struct cairo_state ca;

/* cairo objects */
cairo_surface_t *surface;
cairo_t *cairo;

static void init_cairo(void);
static int ends_with(const char *string, const char *suffix);
static void map_file(void);

static void init_xlib(void)
{
#if defined(USE_X11) && CAIRO_HAS_XLIB_SURFACE
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

    ca.file_name = "<X11>";
    file_type = FTYPE_X11;

    screen_right = screen_left + ca.width;
    screen_bottom = screen_top + ca.height;
#endif
}

static void init_file(void)
{
    int is_vector;
    int do_read = 0;
    int do_map = 0;
    char *p;

    /* set image properties */
    ca.width = screen_width;
    ca.height = screen_height;
    ca.stride = ca.width * 4;

    /* get file name */
    p = getenv("GRASS_PNGFILE");
    if (!p || strlen(p) == 0)
	p = DEFAULT_FILE_NAME;

    ca.file_name = p;

    /* get file type (from extension) */
    if (ca.file_type == FTYPE_X11) ;	/* skip */
    else if (ends_with(ca.file_name, ".ppm"))
	ca.file_type = FTYPE_PPM;
    else if (ends_with(ca.file_name, ".bmp"))
	ca.file_type = FTYPE_BMP;
#if CAIRO_HAS_PNG_FUNCTIONS
    else if (ends_with(ca.file_name, ".png"))
	ca.file_type = FTYPE_PNG;
#endif
#if CAIRO_HAS_PDF_SURFACE
    else if (ends_with(ca.file_name, ".pdf"))
	ca.file_type = FTYPE_PDF;
#endif
#if CAIRO_HAS_PS_SURFACE
    else if (ends_with(ca.file_name, ".ps"))
	ca.file_type = FTYPE_PS;
#endif
#if CAIRO_HAS_SVG_SURFACE
    else if (ends_with(ca.file_name, ".svg"))
	ca.file_type = FTYPE_SVG;
#endif
    else
	G_fatal_error("Unknown file extension: %s", p);
    G_debug(1, "File type: %s (%d)", ca.file_name, ca.file_type);

    switch (ca.file_type) {
    case FTYPE_PDF:
    case FTYPE_PS:
    case FTYPE_SVG:
	is_vector = 1;
	break;
    }

    p = getenv("GRASS_PNG_MAPPED");
    do_map = p && strcmp(p, "TRUE") == 0 && ends_with(ca.file_name, ".bmp");

    p = getenv("GRASS_PNG_READ");
    do_read = p && strcmp(p, "TRUE") == 0;

    if (is_vector) {
	do_read = do_map = 0;
	ca.bgcolor_a = 1.0;
    }

    if (do_read && access(ca.file_name, 0) != 0)
	do_read = 0;

    G_message
	("cairo: collecting to file: %s,\n     GRASS_WIDTH=%d, GRASS_HEIGHT=%d",
	 ca.file_name, ca.width, ca.height);

    if (do_read && do_map)
	map_file();

    if (!ca.mapped && !is_vector)
	ca.grid = G_malloc(ca.height * ca.stride);

    init_cairo();

    if (!do_read && !is_vector) {
	Cairo_Erase();
	ca.modified = 1;
    }

    if (do_read && !ca.mapped)
	cairo_read_image();

    if (do_map && !ca.mapped) {
	cairo_write_image();
	map_file();
	init_cairo();
    }
}

int Cairo_Graph_set(void)
{
    cairo_antialias_t antialias;
    char *p;

    G_gisinit("Cairo driver");
    G_debug(1, "Cairo_Graph_set");

    /* get background color */
    p = getenv("GRASS_BACKGROUNDCOLOR");
    if (p && *p) {
	unsigned int red, green, blue;

	if (sscanf(p, "%02x%02x%02x", &red, &green, &blue) == 3) {
	    ca.bgcolor_r = CAIROCOLOR(red);
	    ca.bgcolor_g = CAIROCOLOR(green);
	    ca.bgcolor_b = CAIROCOLOR(blue);
	}
	else
	    G_fatal_error("Unknown background color: %s", p);
    }
    else
	ca.bgcolor_r = ca.bgcolor_g = ca.bgcolor_b = 1.0;

    /* get background transparency setting */
    p = getenv("GRASS_TRANSPARENT");
    if (p && strcmp(p, "TRUE") == 0)
	ca.bgcolor_a = 0.0;
    else
	ca.bgcolor_a = 1.0;

    p = getenv("GRASS_PNG_AUTO_WRITE");
    ca.auto_write = p && strcmp(p, "TRUE") == 0;

    antialias = CAIRO_ANTIALIAS_DEFAULT;
    p = getenv("GRASS_ANTIALIAS");
    if (p && G_strcasecmp(p, "default") == 0)
	antialias = CAIRO_ANTIALIAS_DEFAULT;
    if (p && G_strcasecmp(p, "none") == 0)
	antialias = CAIRO_ANTIALIAS_NONE;
    if (p && G_strcasecmp(p, "gray") == 0)
	antialias = CAIRO_ANTIALIAS_GRAY;
    if (p && G_strcasecmp(p, "subpixel") == 0)
	antialias = CAIRO_ANTIALIAS_SUBPIXEL;

    p = getenv("GRASS_CAIRO_DRAWABLE");
    if (p)
	init_xlib();
    else
	init_file();

    cairo_set_antialias(cairo, antialias);

    return 0;
}

void Cairo_Graph_close(void)
{
    G_debug(1, "Cairo_Graph_close");

    cairo_write_image();

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
    switch (ca.file_type) {
    case FTYPE_PPM:
    case FTYPE_BMP:
    case FTYPE_PNG:
	surface =
	    (cairo_surface_t *) cairo_image_surface_create_for_data(
		ca.grid, CAIRO_FORMAT_ARGB32, ca.width, ca.height, ca.stride);
	break;
#if CAIRO_HAS_PDF_SURFACE
    case FTYPE_PDF:
	surface =
	    (cairo_surface_t *) cairo_pdf_surface_create(
		ca.file_name, (double) ca.width, (double) ca.height);
	break;
#endif
#if CAIRO_HAS_PS_SURFACE
    case FTYPE_PS:
	surface =
	    (cairo_surface_t *) cairo_ps_surface_create(
		ca.file_name, (double) ca.width, (double) ca.height);
	break;
#endif
#if CAIRO_HAS_SVG_SURFACE
    case FTYPE_SVG:
	surface =
	    (cairo_surface_t *) cairo_svg_surface_create(
		ca.file_name, (double) ca.width, (double) ca.height);
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
    size_t size = HEADER_SIZE + ca.width * ca.height * sizeof(unsigned int);
    void *ptr;
    int fd;

    fd = open(ca.file_name, O_RDWR);
    if (fd < 0)
	return;

    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t) 0);
    if (ptr == MAP_FAILED)
	return;

    if (ca.grid) {
	cairo_destroy(cairo);
	cairo_surface_destroy(surface);
	G_free(ca.grid);
    }
    ca.grid = (char *)ptr + HEADER_SIZE;

    close(fd);

    ca.mapped = 1;
#endif
}
