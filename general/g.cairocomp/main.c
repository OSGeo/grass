/*
 * MODULE:       g.cairocomp
 * AUTHOR(S):    Glynn Clements
 * PURPOSE:      g.cairocomp isn't meant for end users. It's an internal tool for use by
 *               a wx GUI.
 *               g.cairocomp composites a series of X pixmaps.
 * COPYRIGHT:    (C) 2008 by Glynn Clements and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <cairo-xlib-xrender.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/glocale.h>

static int width, height;
static XID output;
static Display *dpy;
static cairo_surface_t *surface;
static cairo_t *cairo;
static Screen *screen;
static XRenderPictFormat *format;
static int depth;

static XID read_xid(const char *filename)
{
    FILE *fp;
    char buf[64];
    long xid;

    fp = fopen(filename, "r");
    if (!fp)
	G_fatal_error(_("Unable to open input file <%s>"), filename);

    if (!fgets(buf, sizeof(buf), fp))
	G_fatal_error(_("Unable to read input file <%s>"), filename);

    if (sscanf(buf, "%li", &xid) != 1)
	G_fatal_error(_("Unable to parse input file <%s>"), filename);

    fclose(fp);

    return (XID) xid;
}

static void write_xid(const char *filename, XID xid)
{
    FILE *fp;
    char buf[64];

    fp = fopen(filename, "w");
    if (!fp)
	G_fatal_error(_("Unable to open output file <%s>"), filename);

    sprintf(buf, "0x%08lx\n", (unsigned long) xid);

    if (fputs(buf, fp) < 0)
	G_fatal_error(_("Unable to write output file <%s>"), filename);

    fclose(fp);
}

static void init_xlib(const char *scr, const char *vis)
{
    XVisualInfo templ;
    XVisualInfo *vinfo;
    int count;
    Visual *visual;
    Pixmap pix;
    int scrn;
    long visid;
    cairo_surface_t *s1, *s2;

    dpy = XOpenDisplay(NULL);
    if (!dpy)
	G_fatal_error(_("Unable to open display"));

    if (scr)
	scrn = atoi(scr);
    else {
	const char *p = getenv("GRASS_CAIRO_SCREEN");
	if (!p || sscanf(p, "%i", &scrn) != 1)
	    scrn = DefaultScreen(dpy);
    }

    screen = ScreenOfDisplay(dpy, scrn);

    if (vis)
	visid = strtoul(vis, NULL, 0);
    else {
	const char *p = getenv("GRASS_CAIRO_VISUAL");
	if (!p || sscanf(p, "%li", &visid) != 1)
	    visid = DefaultVisual(dpy, scrn)->visualid;
    }

    templ.visualid = visid;
    templ.screen = scrn;
    vinfo = XGetVisualInfo(dpy, VisualIDMask|VisualScreenMask, &templ, &count);
    if (!vinfo || !count)
	G_fatal_error(_("Unable to obtain visual"));
    visual = vinfo[0].visual;

    pix = XCreatePixmap(dpy, RootWindow(dpy, scrn), 1, 1, vinfo[0].depth);
    s1 = cairo_xlib_surface_create(dpy, pix, visual, 1, 1);
    s2 = cairo_surface_create_similar(s1, CAIRO_CONTENT_COLOR_ALPHA, 1, 1);
    format = cairo_xlib_surface_get_xrender_format(s2);
    depth = cairo_xlib_surface_get_depth(s2);
    cairo_surface_destroy(s2);
    cairo_surface_destroy(s1);
    XFreePixmap(dpy, pix);

    output = XCreatePixmap(dpy, RootWindow(dpy, scrn), width, height, depth);

    surface = cairo_xlib_surface_create_with_xrender_format(dpy, output, screen, format, width, height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
	G_fatal_error(_("Failed to initialize output surface"));

    cairo = cairo_create(surface);
}

static void fini_xlib(void)
{
    cairo_surface_destroy(surface);
    XFlush(dpy);
    XSetCloseDownMode(dpy, RetainPermanent);
    XCloseDisplay(dpy);
}

static void erase(const char *color)
{
    int r, g, b, a;
    double fr, fg, fb, fa;
    
    a = 255;
    if (sscanf(color, "%d:%d:%d:%d", &r, &g, &b, &a) != 4 ||
        G_str_to_color(color, &r, &g, &b) != 1)
	G_fatal_error(_("Invalid color: %s"), color);
    
    fr = r / 255.0;
    fg = g / 255.0;
    fb = b / 255.0;
    fa = a / 255.0;

    cairo_save(cairo);
    cairo_set_source_rgba(cairo, fr, fg, fb, fa);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);
    cairo_restore(cairo);
}

static void overlay(XID src, float alpha)
{
    cairo_surface_t *src_surf;

    src_surf = cairo_xlib_surface_create_with_xrender_format(
	dpy, src, screen, format, width, height);
    if (cairo_surface_status(src_surf) != CAIRO_STATUS_SUCCESS)
	G_fatal_error(_("Failed to initialize input surface"));

    cairo_save(cairo);
    cairo_set_operator(cairo, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(cairo, src_surf, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cairo), CAIRO_FILTER_NEAREST);
    cairo_paint_with_alpha(cairo, alpha);
    cairo_restore(cairo);

    cairo_surface_destroy(src_surf);
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *in, *out, *visual, *screen, *alpha, *width, *height, *bg;
    } opt;
    struct Flag *flag_d;
    int i;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("display"));
    module->description = _("Overlays multiple X Pixmaps.");

    opt.in = G_define_standard_option(G_OPT_F_INPUT);
    opt.in->required = YES;
    opt.in->multiple = YES;
    opt.in->description = _("Name of input file(s)");

    opt.out = G_define_standard_option(G_OPT_F_OUTPUT);
    opt.out->required = YES;

    opt.visual = G_define_option();
    opt.visual->key = "visual";
    opt.visual->type = TYPE_INTEGER;
    opt.visual->required = NO;
    opt.visual->description = _("Output Visual XID");

    opt.screen = G_define_option();
    opt.screen->key = "screen";
    opt.screen->type = TYPE_INTEGER;
    opt.screen->required = NO;
    opt.screen->description = _("Output screen");

    opt.alpha = G_define_option();
    opt.alpha->key = "opacity";
    opt.alpha->type = TYPE_DOUBLE;
    opt.alpha->multiple = YES;
    opt.alpha->description = _("Layer opacities");

    opt.width = G_define_option();
    opt.width->key = "width";
    opt.width->type = TYPE_INTEGER;
    opt.width->required = YES;
    opt.width->description = _("Image width");

    opt.height = G_define_option();
    opt.height->key = "height";
    opt.height->type = TYPE_INTEGER;
    opt.height->required = YES;
    opt.height->description = _("Image height");

    opt.bg = G_define_standard_option(G_OPT_C_BG);
    opt.bg->label = _("Background color (R:G:B:A)");
    opt.bg->answer = NULL;

    flag_d = G_define_flag();
    flag_d->key = 'd';
    flag_d->description = _("Don't composite; just delete input Pixmaps");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    width = atoi(opt.width->answer);
    height = atoi(opt.height->answer);

    if (flag_d->answer) {
	dpy = XOpenDisplay(NULL);
	if (!dpy)
	    G_fatal_error(_("Unable to open display"));
	for (i = 0; opt.in->answers[i]; i++) {
	    unsigned long xid = read_xid(opt.in->answers[i]);
	    XKillClient(dpy, xid);
	}
	XFlush(dpy);
	XCloseDisplay(dpy);
	return 0;
    }

    init_xlib(opt.screen->answer, opt.visual->answer);

    if (opt.bg->answer)
	erase(opt.bg->answer);

    for (i = 0; opt.in->answers[i]; i++) {
	XID input = read_xid(opt.in->answers[i]);
	double alpha;

	if (opt.alpha->answer && !opt.alpha->answers[i])
	    G_fatal_error(
		_("input= and opacity= must have the same number of values"));

	alpha = opt.alpha->answer ? atof(opt.alpha->answers[i]) : 1.0;

	overlay(input, alpha);
    }

    write_xid(opt.out->answer, output);

    fini_xlib();

    return 0;
}

