
/****************************************************************************
 *
 * MODULE:       ximgview
 * AUTHOR(S):    Glynn Clements
 * PURPOSE:      View BMP images from the PNG driver
 * COPYRIGHT:    (C) 2007 Glynn Clements
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#define HEADER_SIZE 64

Display *dpy;
int scrn;
Window grwin;
XWindowAttributes xwa;

static int evmask = ExposureMask | StructureNotifyMask;
static int w_width, w_height;
static int i_width, i_height;
static unsigned long last;
static double fraction;

static void *imgbuf;
static void *xbuf;
static XImage *ximg;
static GC gc;

extern Colormap InitColorTableFixed(Colormap cmap);
extern unsigned long find_color(unsigned int r, unsigned int g,
				unsigned int b);

static void create_window(void)
{
    XSetWindowAttributes xswa;
    Colormap fixedcmap;

    dpy = XOpenDisplay(NULL);
    if (!dpy)
	G_fatal_error(_("Unable to open display"));

    scrn = DefaultScreen(dpy);

    xswa.event_mask = evmask;
    xswa.backing_store = NotUseful;
    xswa.background_pixel = BlackPixel(dpy, scrn);

    grwin = XCreateWindow(dpy, RootWindow(dpy, scrn),
			  0, 0,
			  800, 600,
			  0,
			  DefaultDepth(dpy, scrn),
			  InputOutput,
			  DefaultVisual(dpy, scrn),
			  CWEventMask | CWBackingStore | CWBackPixel, &xswa);

    XMapWindow(dpy, grwin);

    if (!XGetWindowAttributes(dpy, grwin, &xwa))
	G_fatal_error(_("Unable to get window attributes"));

    fixedcmap = InitColorTableFixed(DefaultColormap(dpy, scrn));

    XSetWindowColormap(dpy, grwin, fixedcmap);

    gc = XCreateGC(dpy, grwin, 0UL, NULL);

    xbuf = G_malloc(i_width * i_height * 4);
    ximg = XCreateImage(dpy, xwa.visual, xwa.depth, ZPixmap,
			0, xbuf, i_width, i_height, 32, 0);

    w_width = xwa.width;
    w_height = xwa.height;

    XFlush(dpy);
}

static void draw(void)
{
    int x0 = (w_width - i_width) / 2;
    int y0 = (w_height - i_height) / 2;
    const unsigned char *p = imgbuf;
    int row, col;

    for (row = 0; row < i_height; row++) {
	for (col = 0; col < i_width; col++) {
	    unsigned char b = *p++;
	    unsigned char g = *p++;
	    unsigned char r = *p++;
	    unsigned char a = *p++;
	    unsigned long c = find_color(r, g, b);

	    XPutPixel(ximg, col, row, c);
	}
    }

    XPutImage(dpy, grwin, gc, ximg, 0, 0, x0, y0, i_width, i_height);
    XSync(dpy, False);
}

static void redraw(void)
{
    struct timeval tv0, tv1;

    gettimeofday(&tv0, NULL);

    draw();

    gettimeofday(&tv1, NULL);
    last = (tv1.tv_sec - tv0.tv_sec) * 1000000L + (tv1.tv_usec - tv0.tv_usec);
}

static void dummy_handler(int sig)
{
}

static void main_loop(void)
{
    int xfd = ConnectionNumber(dpy);
    struct sigaction act;

    act.sa_handler = &dummy_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, NULL);

    for (;;) {
	fd_set waitset;
	struct timeval tv;
	unsigned long delay;

	while (XPending(dpy) > 0) {
	    XEvent event;

	    XNextEvent(dpy, &event);

	    switch (event.type) {
	    case Expose:
		draw();
		break;
	    case ConfigureNotify:
		w_width = event.xconfigure.width;
		w_height = event.xconfigure.height;
		break;
	    }
	}

	if (fraction > 0.001)
	    delay = (unsigned long)(last / fraction);

	tv.tv_sec = delay / 1000000;
	tv.tv_usec = delay % 1000000;

	FD_ZERO(&waitset);
	FD_SET(xfd, &waitset);
	errno = 0;
	if (select(FD_SETSIZE, &waitset, NULL, NULL, &tv) < 0 && errno != EINTR)
	    continue;

	if (!FD_ISSET(xfd, &waitset) || errno == EINTR)
	    redraw();
    }
}

static unsigned int get_2(const unsigned char **q)
{
    const unsigned char *p = *q;
    unsigned int n = (p[0] << 0) | (p[1] << 8);

    *q += 2;
    return n;
}

static unsigned int get_4(const unsigned char **q)
{
    const unsigned char *p = *q;
    unsigned int n = (p[0] << 0) | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);

    *q += 4;
    return n;
}

static int read_bmp_header(const unsigned char *p)
{
    int size;

    if (*p++ != 'B')
	return 0;
    if (*p++ != 'M')
	return 0;

    size = get_4(&p);

    get_4(&p);

    if (get_4(&p) != HEADER_SIZE)
	return 0;

    if (get_4(&p) != 40)
	return 0;

    i_width = get_4(&p);
    i_height = -get_4(&p);

    get_2(&p);
    if (get_2(&p) != 32)
	return 0;

    if (get_4(&p) != 0)
	return 0;
    if (get_4(&p) != i_width * i_height * 4)
	return 0;

    if (size != HEADER_SIZE + i_width * i_height * 4)
	return 0;

    get_4(&p);
    get_4(&p);
    get_4(&p);
    get_4(&p);

    return 1;
}

static void map_file(const char *filename)
{
    char header[HEADER_SIZE];
    size_t size;
    void *ptr;
    int fd;

    fd = open(filename, O_RDONLY);
    if (fd < 0)
	G_fatal_error(_("Unable to open image file"));

    if (read(fd, header, sizeof(header)) != sizeof(header))
	G_fatal_error(_("Unable to read BMP header"));

    if (!read_bmp_header(header))
	G_fatal_error(_("Invalid BMP header"));

    size = HEADER_SIZE + i_width * i_height * 4;

    ptr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, (off_t) 0);
    if (ptr == MAP_FAILED)
	G_fatal_error(_("Unable to map image file"));

    imgbuf = (char *)ptr + HEADER_SIZE;

    close(fd);
}

int main(int argc, char **argv)
{
    struct GModule *module;
    struct
    {
	struct Option *image, *percent;
    } opt;
    const char *filename;
    int percent;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("graphics"));
    G_add_keyword(_("raster"));
    G_add_keyword(_("vector"));
    G_add_keyword(_("visualization"));
    module->description = _("View BMP images from the PNG driver.");

    opt.image = G_define_standard_option(G_OPT_F_OUTPUT);
    opt.image->key = "image";
    opt.image->required = YES;
    opt.image->description = _("Name for output image file");

    opt.percent = G_define_option();
    opt.percent->key = "percent";
    opt.percent->type = TYPE_INTEGER;
    opt.percent->required = NO;
    opt.percent->multiple = NO;
    opt.percent->description = _("Percentage of CPU time to use");
    opt.percent->answer = "10";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    filename = opt.image->answer;
    percent = atoi(opt.percent->answer);
    fraction = percent / 100.0;

    map_file(filename);
    create_window();
    main_loop();

    XCloseDisplay(dpy);

    return EXIT_SUCCESS;
}
