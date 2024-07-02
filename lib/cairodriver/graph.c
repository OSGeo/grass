/*!
   \file lib/cairodriver/graph.c

   \brief GRASS cairo display driver - driver settings

   (C) 2007-2008, 2011 by Lars Ahlzen and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Lars Ahlzen <lars ahlzen.com> (original contributor)
   \author Glynn Clements
 */

#include "cairodriver.h"

#if CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif
#if CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif
#if CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif
#if CAIRO_HAS_XLIB_XRENDER_SURFACE
#include <cairo-xlib.h>
#include <cairo-xlib-xrender.h>
#endif

#include <unistd.h>
#ifndef _WIN32
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

#include <grass/colors.h>
#include <grass/glocale.h>

struct cairo_state ca;

/* cairo objects */
cairo_surface_t *surface;
cairo_t *cairo;

static void init_cairo(void);
static int ends_with(const char *string, const char *suffix);
static void map_file(void);

#if CAIRO_HAS_XLIB_XRENDER_SURFACE
static void init_xlib(void)
{
    char *p;
    unsigned long xid;
    XVisualInfo templ;
    XVisualInfo *vinfo;
    int count;
    Visual *visual;
    int scrn;
    Pixmap pix;
    cairo_surface_t *s1, *s2;

    ca.dpy = XOpenDisplay(NULL);
    if (!ca.dpy)
        G_fatal_error(_("Unable to open display"));

    p = getenv("GRASS_RENDER_CAIRO_SCREEN");
    if (!p || sscanf(p, "%i", &scrn) != 1) {
        G_debug(1, "cairo: GRASS_RENDER_CAIRO_SCREEN=%s", p);
        scrn = DefaultScreen(ca.dpy);
    }

    p = getenv("GRASS_RENDER_CAIRO_VISUAL");
    if (!p || sscanf(p, "%lu", &xid) != 1) {
        G_debug(1, "cairo: GRASS_RENDER_CAIRO_VISUAL=%s", p);
        xid = DefaultVisual(ca.dpy, scrn)->visualid;
    }
    templ.visualid = xid;
    templ.screen = scrn;

    vinfo =
        XGetVisualInfo(ca.dpy, VisualIDMask | VisualScreenMask, &templ, &count);
    if (!vinfo || !count)
        G_fatal_error(_("Unable to obtain visual"));
    visual = vinfo[0].visual;

    ca.screen = ScreenOfDisplay(ca.dpy, scrn);
    pix = XCreatePixmap(ca.dpy, RootWindow(ca.dpy, scrn), 1, 1, vinfo[0].depth);
    s1 = cairo_xlib_surface_create(ca.dpy, pix, visual, 1, 1);
    s2 = cairo_surface_create_similar(s1, CAIRO_CONTENT_COLOR_ALPHA, 1, 1);
    ca.format = cairo_xlib_surface_get_xrender_format(s2);
    ca.depth = cairo_xlib_surface_get_depth(s2);
    cairo_surface_destroy(s2);
    cairo_surface_destroy(s1);
    XFreePixmap(ca.dpy, pix);

    if (!ca.win)
        ca.win = XCreatePixmap(ca.dpy, RootWindow(ca.dpy, scrn), ca.width,
                               ca.height, ca.depth);
}

static void fini_xlib(void)
{
    XSetCloseDownMode(ca.dpy, RetainTemporary);
    XCloseDisplay(ca.dpy);
}
#endif

static void init_file(void)
{
    int is_vector = 0;

#if CAIRO_HAS_XLIB_XRENDER_SURFACE
    int is_xlib = 0;
#endif
    int do_read = 0;
    int do_map = 0;
    char *p;

    /* set image properties */
    ca.width = screen_width;
    ca.height = screen_height;
    ca.stride = ca.width * 4;

    /* get file name */
    p = getenv("GRASS_RENDER_FILE");
    if (!p || strlen(p) == 0)
        p = DEFAULT_FILE_NAME;
    G_debug(1, "cairo: GRASS_RENDER_FILE=%s", p);

    ca.file_name = p;

    /* get file type (from extension) */
    if (ends_with(ca.file_name, ".ppm"))
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
#if CAIRO_HAS_XLIB_XRENDER_SURFACE
    else if (ends_with(ca.file_name, ".xid"))
        ca.file_type = FTYPE_X11;
#endif
    else
        G_fatal_error(_("Unknown file extension: %s"), p);
    G_debug(1, "cairo: file type=%d", ca.file_type);

    switch (ca.file_type) {
    case FTYPE_PDF:
    case FTYPE_PS:
    case FTYPE_SVG:
        is_vector = 1;
        break;
#if CAIRO_HAS_XLIB_XRENDER_SURFACE
    case FTYPE_X11:
        is_xlib = 1;
        break;
#endif
    }

    p = getenv("GRASS_RENDER_FILE_MAPPED");
    do_map = p && strcmp(p, "TRUE") == 0 && ends_with(ca.file_name, ".bmp");
    G_debug(1, "cairo: GRASS_RENDER_FILE_MAPPED=%d", do_map);

    p = getenv("GRASS_RENDER_FILE_READ");
    do_read = p && strcmp(p, "TRUE") == 0;
    G_debug(1, "cairo: GRASS_RENDER_FILE_READ=%d", do_read);

    if (is_vector) {
        do_read = do_map = 0;
        ca.bgcolor_a = 1.0;
    }

    if (do_read && access(ca.file_name, 0) != 0)
        do_read = 0;

    G_verbose_message(_("cairo: collecting to file '%s'"), ca.file_name);
    G_verbose_message(_("cairo: image size %dx%d"), ca.width, ca.height);

    if (do_read && do_map)
        map_file();

#if CAIRO_HAS_XLIB_XRENDER_SURFACE
    if (is_xlib) {
        if (do_read)
            cairo_read_xid();
        else
            ca.win = 0;
        init_xlib();
        ca.mapped = 1;
    }
#endif

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

/*!
   \brief Initialize driver

   Set background color, transparency, drawable, antialias mode, etc.

   \return 0
 */
int Cairo_Graph_set(void)
{
    cairo_antialias_t antialias;
    char *p;

    G_gisinit("Cairo driver");

    /* get background color */
    p = getenv("GRASS_RENDER_BACKGROUNDCOLOR");
    if (p && *p) {
        unsigned int red, green, blue;

        if (sscanf(p, "%02x%02x%02x", &red, &green, &blue) == 3 ||
            G_str_to_color(p, (int *)&red, (int *)&green, (int *)&blue) == 1) {
            ca.bgcolor_r = CAIROCOLOR(red);
            ca.bgcolor_g = CAIROCOLOR(green);
            ca.bgcolor_b = CAIROCOLOR(blue);
        }
        else
            G_fatal_error("Unknown background color: %s", p);
        G_debug(1, "cairo: GRASS_RENDER_BACKGROUNDCOLOR=%s", p);
    }
    else
        ca.bgcolor_r = ca.bgcolor_g = ca.bgcolor_b = 1.0;

    /* get background transparency setting */
    p = getenv("GRASS_RENDER_TRANSPARENT");
    if (p && strcmp(p, "TRUE") == 0)
        ca.bgcolor_a = 0.0;
    else
        ca.bgcolor_a = 1.0;
    G_debug(1, "cairo: GRASS_RENDER_TRANSPARENT=%s", p ? p : "FALSE");

    antialias = CAIRO_ANTIALIAS_DEFAULT;
    p = getenv("GRASS_RENDER_ANTIALIAS");
    if (p && G_strcasecmp(p, "default") == 0)
        antialias = CAIRO_ANTIALIAS_DEFAULT;
    if (p && G_strcasecmp(p, "none") == 0)
        antialias = CAIRO_ANTIALIAS_NONE;
    if (p && G_strcasecmp(p, "gray") == 0)
        antialias = CAIRO_ANTIALIAS_GRAY;
    if (p && G_strcasecmp(p, "subpixel") == 0)
        antialias = CAIRO_ANTIALIAS_SUBPIXEL;
    G_debug(1, "cairo: GRASS_RENDER_ANTIALIAS=%s", p ? p : "FALSE");

    init_file();

    cairo_set_antialias(cairo, antialias);

    return 0;
}

/*!
   \brief Get render file

   \return file name
 */
const char *Cairo_Graph_get_file(void)
{
    return ca.file_name;
}

/*!
   \brief Close driver
 */
void Cairo_Graph_close(void)
{
    G_debug(1, "Cairo_Graph_close");

#if CAIRO_HAS_XLIB_XRENDER_SURFACE
    if (ca.file_type == FTYPE_X11) {
        XFlush(cairo_xlib_surface_get_display(surface));
        ca.mapped = 0;
    }
#endif

    cairo_write_image();

    if (cairo) {
        cairo_destroy(cairo);
        cairo = NULL;
    }
    if (surface) {
        cairo_surface_destroy(surface);
        surface = NULL;
    }

#if CAIRO_HAS_XLIB_XRENDER_SURFACE
    if (ca.file_type == FTYPE_X11)
        fini_xlib();
#endif
}

static void init_cairo(void)
{
    G_debug(1, "init_cairo");

    /* create cairo surface */
    switch (ca.file_type) {
    case FTYPE_PPM:
    case FTYPE_BMP:
    case FTYPE_PNG:
        surface = (cairo_surface_t *)cairo_image_surface_create_for_data(
            ca.grid, CAIRO_FORMAT_ARGB32, ca.width, ca.height, ca.stride);
        break;
#if CAIRO_HAS_PDF_SURFACE
    case FTYPE_PDF:
        surface = (cairo_surface_t *)cairo_pdf_surface_create(
            ca.file_name, (double)ca.width, (double)ca.height);
        break;
#endif
#if CAIRO_HAS_PS_SURFACE
    case FTYPE_PS:
        surface = (cairo_surface_t *)cairo_ps_surface_create(
            ca.file_name, (double)ca.width, (double)ca.height);
        break;
#endif
#if CAIRO_HAS_SVG_SURFACE
    case FTYPE_SVG:
        surface = (cairo_surface_t *)cairo_svg_surface_create(
            ca.file_name, (double)ca.width, (double)ca.height);
        break;
#endif
#if CAIRO_HAS_XLIB_XRENDER_SURFACE
    case FTYPE_X11:
        surface =
            (cairo_surface_t *)cairo_xlib_surface_create_with_xrender_format(
                ca.dpy, ca.win, ca.screen, ca.format, ca.width, ca.height);
        break;
#endif
    default:
        G_fatal_error(_("Unknown Cairo surface type"));
        break;
    }

    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
        G_fatal_error(_("Failed to initialize Cairo surface"
                        " (width: %d, height: %d): %s"),
                      ca.width, ca.height,
                      cairo_status_to_string(cairo_surface_status(surface)));

    cairo = cairo_create(surface);
}

/* Returns TRUE if string ends with suffix (case insensitive) */
static int ends_with(const char *string, const char *suffix)
{
    if (strlen(string) < strlen(suffix))
        return FALSE;

    return G_strcasecmp(suffix, string + strlen(string) - strlen(suffix)) == 0;
}

static void map_file(void)
{
#ifndef _WIN32
    size_t size = HEADER_SIZE + ca.width * ca.height * sizeof(unsigned int);
    void *ptr;
    int fd;

    fd = open(ca.file_name, O_RDWR);
    if (fd < 0)
        return;

    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)0);
    if (ptr == MAP_FAILED)
        return;

    if (ca.grid) {
        cairo_destroy(cairo);
        cairo_surface_destroy(surface);
        G_free(ca.grid);
    }
    ca.grid = (unsigned char *)ptr + HEADER_SIZE;

    close(fd);

    ca.mapped = 1;
#endif
}
