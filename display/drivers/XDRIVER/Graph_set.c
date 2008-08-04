
/* Changed for truecolor 24bit support by
 * Roberto Flor/ITC-Irst, Trento, Italy
 * August 1999
 *
 * Heavily modified by Glynn Clements, May 2001
 */

#include <grass/config.h>

/* This driver extensively updated by P. Thompson
 * (phils@athena.mit.edu) on 9/13/90 Driver modified to work with
 * Decstation X11r3 by David B. Satnik on 8/90 */
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "includes.h"
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <grass/gis.h>
#include "icon.bit"
#include <grass/winname.h>
#include <grass/glocale.h>
#include "XDRIVER.h"

/* This program is a rewrite of the original Grah_Set from the GRASS
 * 3.0 version. All suncore and sunview related stuff (which was the
 * bulk of the original code) has been replaced by X11 library calls.
 * All non-suncore code has been retained. */

/* declare global variables */

const char *monitor_name;

Display *dpy;
Window grwin;

Visual *use_visual;
int use_bit_depth;

int scrn;
Screen *use_screen;
GC gc;
Colormap fixedcmap;
Cursor cur_xh, cur_clock;
u_long gemask = StructureNotifyMask;
Pixmap bkupmap;
int truecolor;

int external_window;

unsigned long *xpixels;		/* lookup table for FIXED color mode */

#ifdef X11R3

/* compatibility functions for X11R3 */

static Status
XSetWMProtocols(Display * dpy, Window w, Atom * protocols, int count)
{
    Atom wmProtocols;

    wmProtocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
    if (!wmProtocols)
	return 0;

    XChangeProperty(dpy, w, wmProtocols, XA_ATOM, 32, PropModeReplace,
		    (unsigned char *)protocols, count);
    return 1;
}

XSizeHints *XAllocSizeHints(void)
{
    return (XSizeHints *) G_malloc(sizeof(XSizeHints));
}

XWMHints *XAllocWMHints(void)
{
    return (XWMHints *) G_malloc(sizeof(XWMHints));
}

XClassHint *XAllocClassHint(void)
{
    return (XClassHint *) G_malloc(sizeof(XClassHint));
}

#endif

static RETSIGTYPE sigint(int sig)
{
    XD_Graph_close();
    exit(1);
}

static void find_truecolor_visual(void)
{
    static int search_mask = VisualClassMask | VisualScreenMask;
    XVisualInfo search_template;
    XVisualInfo *mvisual_info;
    int num_visuals;
    int vis_num;
    int highest_bit_depth = 0;

    search_template.class = TrueColor;
    search_template.screen = scrn;

    mvisual_info =
	XGetVisualInfo(dpy, search_mask, &search_template, &num_visuals);

    G_message(_("found %d visuals of type TrueColor"), num_visuals);

    G_message(_("searching for highest bit depth"));

    for (vis_num = 0; vis_num < num_visuals; vis_num++) {
	if (mvisual_info[vis_num].depth <= highest_bit_depth)
	    continue;

	use_visual = mvisual_info[vis_num].visual;
	use_bit_depth = mvisual_info[vis_num].depth;
	highest_bit_depth = use_bit_depth;
    }

    XFree(mvisual_info);

    if (highest_bit_depth == 0)
	G_warning("unable to find a TrueColor visual\n");
    else
	G_message(_("selected %d bit depth"), use_bit_depth);
}

static void use_window(int win_id)
{
    XWindowAttributes xwa;

    external_window = 1;

    grwin = (Window) win_id;
    XSelectInput(dpy, grwin, gemask);

    if (!XGetWindowAttributes(dpy, grwin, &xwa))
	G_fatal_error("Graph_Set: cannot get window attributes\n");

    use_screen = xwa.screen;
    scrn = XScreenNumberOfScreen(xwa.screen);

    use_visual = xwa.visual;

    use_bit_depth = xwa.depth;
}

static void create_window(int argc, char **argv)
{
    static const char *const classname[6] = {
	"StaticGray", "GrayScale",
	"StaticColor", "PseudoColor",
	"TrueColor", "DirectColor"
    };
    XSetWindowAttributes xswa;	/* Set Window Attribute struct */
    Atom closedownAtom;

#ifndef X11R3
    XTextProperty windowName, iconName;
#endif /* X11R3 */
    XSizeHints *szhints;
    XClassHint *clshints;
    XWMHints *wmhints;
    char title[1024];
    char *p;

    external_window = 0;

    /* scrn is the screen number */
    scrn = DefaultScreen(dpy);
    /* use_screen is a pointer to that screen structure */
    use_screen = ScreenOfDisplay(dpy, scrn);

    use_visual = NULL;

    /* special flag to indicate a search for a True Color Display */
    p = getenv("XDRIVER_TRUECOLOR");
    if (p && strcmp(p, "TRUE") == 0)
	find_truecolor_visual();

    /*  If we can't find a TrueColor visual then use the default visual     */
    if (!use_visual) {
	use_visual = DefaultVisual(dpy, scrn);
	use_bit_depth = DefaultDepth(dpy, scrn);
	G_message(_("using default visual which is %s"),
		  classname[use_visual->class]);
    }

    /* Deal with providing the window with an initial size.
     * Window is resizable */
    szhints = XAllocSizeHints();

    szhints->flags = USSize;
    szhints->height = screen_bottom - screen_top;
    szhints->width = screen_right - screen_left;

    /* Create the Window with the information in the XSizeHints */

    xswa.event_mask = gemask;
    xswa.backing_store = NotUseful;

    grwin = XCreateWindow(dpy, RootWindow(dpy, scrn),
			  0, 0,
			  (unsigned)szhints->width,
			  (unsigned)szhints->height,
			  0,
			  use_bit_depth,
			  InputOutput,
			  use_visual, (CWEventMask | CWBackingStore), &xswa);

    /* properties for window manager */
    wmhints = XAllocWMHints();
    wmhints->icon_pixmap = XCreateBitmapFromData(dpy, grwin, icon_bits,
						 icon_width, icon_height);
    wmhints->flags |= IconPixmapHint;

    clshints = XAllocClassHint();
    clshints->res_name = NULL;
    clshints->res_class = WIN_NAME;

#ifndef X11R3
    sprintf(title, "GRASS %s - Monitor: %s - Location: %s", WIN_NAME,
	    monitor_name, G_location());

    iconName.encoding = XA_STRING;
    iconName.format = 8;
    iconName.value = (u_char *) title;
    iconName.nitems = strlen((char *)iconName.value);

    windowName.encoding = iconName.encoding = XA_STRING;
    windowName.format = iconName.format = 8;
    windowName.value = (u_char *) title;
    windowName.nitems = strlen((char *)windowName.value);

    XSetWMProperties(dpy, grwin, &windowName, &iconName, argv, argc,
		     szhints, wmhints, clshints);
#endif

    closedownAtom = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, grwin, &closedownAtom, 1);

    /* Map the window to make it visible. This causes an expose event */
    XMapWindow(dpy, grwin);
}

static Cursor create_cross_cursor(void)
{
    static int width = 16;
    static int height = 16;
    static int x0 = 7;
    static int y0 = 7;

    /* Old cursor */
    /*
       static const unsigned char pix_data[] = {
       0x00, 0x00, 0xa0, 0x02, 0xa0, 0x02, 0xa0, 0x02,
       0xa0, 0x02, 0xbe, 0x3e, 0x80, 0x00, 0x7e, 0x3f,
       0x80, 0x00, 0xbe, 0x3e, 0xa0, 0x02, 0xa0, 0x02,
       0xa0, 0x02, 0xa0, 0x02, 0x00, 0x00, 0x00, 0x00
       };
       static const unsigned char mask_data[] = {
       0x00, 0x00, 0xe0, 0x03, 0xe0, 0x03, 0xe0, 0x03,
       0xe0, 0x03, 0xfe, 0x3f, 0xfe, 0x3f, 0x7e, 0x3f,
       0xfe, 0x3f, 0xfe, 0x3f, 0xe0, 0x03, 0xe0, 0x03,
       0xe0, 0x03, 0xe0, 0x03, 0x00, 0x00, 0x00, 0x00
       };
     */
    static const unsigned char pix_data[] = {
	0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x7c,
	0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00,
	0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00
    };
    static const unsigned char mask_data[] = {
	0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01,
	0xc0, 0x01, 0x00, 0x00, 0x1f, 0x7c, 0x1f, 0x7c,
	0x1f, 0x7c, 0x00, 0x00, 0xc0, 0x01, 0xc0, 0x01,
	0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0x00, 0x00,
    };
    Pixmap pix, mask;
    XColor fg, bg;

    fg.pixel = BlackPixel(dpy, scrn);
    XQueryColor(dpy, fixedcmap, &fg);

    bg.pixel = WhitePixel(dpy, scrn);
    XQueryColor(dpy, fixedcmap, &bg);

    pix = XCreateBitmapFromData(dpy, grwin, pix_data, width, height);
    mask = XCreateBitmapFromData(dpy, grwin, mask_data, width, height);

    return XCreatePixmapCursor(dpy, pix, mask, &fg, &bg, x0, y0);
}

int XD_Graph_set(int argc, char **argv)
{
    XWindowAttributes xwa;	/* Get Window Attribute struct */
    const char *privcmap;
    char *p;
    int win_id;

    monitor_name = argv[1];

    /* Open the display using the $DISPLAY environment variable to
     * locate the X server. Return 0 if cannot open. */
    if (!(dpy = XOpenDisplay(NULL)))
	G_fatal_error("Graph_Set: can't open Display %s\n",
		      XDisplayName(NULL));

    privcmap = getenv("XDRIVER_PRIVATE_CMAP");

    if ((p = getenv("XDRIVER_WINDOW")) && sscanf(p, "%i", &win_id) == 1)
	use_window(win_id);
    else
	create_window(argc, argv);

    /* this next bit forces the use of a private colormap                */
    /* a must have for read only visuals                                 */
    /* since we could allow any type to be selected above assume that    */
    /* if this is not the default visual we must use a private colormap  */

    if (use_visual != DefaultVisual(dpy, scrn) || (privcmap && privcmap[0]))
	fixedcmap = XCreateColormap(dpy, DefaultRootWindow(dpy),
				    use_visual, AllocNone);
    else
	fixedcmap = DefaultColormap(dpy, scrn);

    fixedcmap = init_color_table(fixedcmap);

    G_message("ncolors: %d", NCOLORS);

    XSetWindowColormap(dpy, grwin, fixedcmap);

    /* Create the cursors to be used later */

    cur_xh = create_cross_cursor();
    cur_clock = XCreateFontCursor(dpy, XC_watch);

    /* Create the GC. */
    gc = XCreateGC(dpy, grwin, 0UL, NULL);

    /* Find out how big the window really is (in case window manager
     * overrides our request) and set the SCREEN values. */
    screen_left = screen_top = 0;
    if (!XGetWindowAttributes(dpy, grwin, &xwa))
	G_fatal_error("Graph_Set: cannot get window attributes\n");

    screen_right = xwa.width;
    screen_bottom = xwa.height;

    /* Now create a pixmap that will contain same contents as the
     * window. It will be used to redraw from after expose events */
    bkupmap = XCreatePixmap(dpy, grwin, xwa.width, xwa.height, xwa.depth);
    XSetWindowBackgroundPixmap(dpy, grwin, bkupmap);
    XSetForeground(dpy, gc, WhitePixel(dpy, scrn));
    XFillRectangle(dpy, bkupmap, gc, 0, 0, xwa.width, xwa.height);

    XSetBackground(dpy, gc, BlackPixel(dpy, scrn));
    XSetForeground(dpy, gc, WhitePixel(dpy, scrn));

    XClearWindow(dpy, grwin);

    /* prepare to catch signals */
    signal(SIGHUP, sigint);
    signal(SIGINT, sigint);
    signal(SIGQUIT, sigint);
    signal(SIGILL, sigint);
    signal(SIGTSTP, SIG_IGN);

    XFlush(dpy);
    return 0;
}
