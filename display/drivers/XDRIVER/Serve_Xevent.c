#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef __MINGW32__
#include <process.h>
#else
#include <sys/wait.h>
#endif
#include "includes.h"
#include <grass/colors.h>
#include "pad.h"
#include <grass/gis.h>
#include <grass/glocale.h>
#include "XDRIVER.h"

static void spawn_redraw_process(void);
static void check_redraw_process(void);
static void handle_resize_event(void);
static void check_flush(void);
static void set_busy(int);
static void set_title_busy(int);

pid_t redraw_pid;

int needs_flush;

int get_xevent(long event_mask, XEvent * event)
{
    int input_fd = LIB_command_get_input();
    int display_fd = ConnectionNumber(dpy);

    for (;;) {
	fd_set waitset;
	struct timeval tv;

	if (XCheckWindowEvent(dpy, grwin, event_mask, event))
	    return 1;

	tv.tv_sec = 0;
	tv.tv_usec = 200;

	FD_ZERO(&waitset);
	FD_SET(input_fd, &waitset);
	FD_SET(display_fd, &waitset);
	if (select(FD_SETSIZE, &waitset, NULL, NULL, &tv) < 0) {
	    perror("monitor: get_event: select");
	    return 0;
	}

	if (FD_ISSET(input_fd, &waitset))
	    return 0;
    }
}

int service_xevent(int opened)
{
    static int do_resize;
    Atom WM_DELETE_WINDOW;
    XEvent event;

    check_redraw_process();

    while (XPending(dpy)) {
	/* NOTE: This won't die if server terminates */
	XNextEvent(dpy, &event);
	switch (event.type) {
	case ConfigureNotify:
	    if (event.xconfigure.width != screen_right ||
		event.xconfigure.height != screen_bottom)
		do_resize = 1;	/* group requests into one */
	    break;

	case ClientMessage:
	    WM_DELETE_WINDOW = XInternAtom(event.xclient.display,
					   "WM_DELETE_WINDOW", False);
	    if (event.xclient.data.l[0] != WM_DELETE_WINDOW)
		break;
	    XD_Graph_close();
	    exit(0);
	    break;
	}
    }				/* while() */

    /* Now process resize or expose events */
    if (do_resize && !redraw_pid && !opened) {
	spawn_redraw_process();
	handle_resize_event();
	do_resize = 0;
    }

    check_flush();

    return 0;
}

int _time_stamp(PAD * pad)
{
    append_item(pad, "time", "1", 1);

    return 0;
}

static void check_redraw_process(void)
{
    int status;
    pid_t pid;

    if (!redraw_pid)
	return;

    pid = waitpid(redraw_pid, &status, WNOHANG);
    if (pid < 0) {
	perror("Monitor: check_redraw_process: waitpid");
	return;
    }

    if (pid == 0)
	return;

    if (pid != redraw_pid) {
	G_warning(_("Monitor: waitpid: expected %d but got %d"),
		  redraw_pid, pid);
	return;
    }

    set_busy(0);
    redraw_pid = 0;
}

static LIST *list_push(char *s, LIST * tail)
{
    LIST *l = (LIST *) G_malloc(sizeof(LIST));

    l->value = G_store(s);
    l->next = tail;
    return l;
}

static LIST *list_copy(LIST * list, LIST * tail)
{
    if (!list)
	return tail;

    return list_push(list->value, list_copy(list->next, tail));
}

static void list_free(LIST * l)
{
    if (!l)
	return;
    if (l->value)
	G_free(l->value);
    list_free(l->next);
    G_free(l);
}

static void spawn_redraw_process(void)
{
    char buff[1024];
    pid_t pid;
    LIST *commands = NULL;
    PAD *pad;

    if (redraw_pid)
	return;

    for (pad = pad_list(); pad != NULL; pad = pad->next) {
	ITEM *list, *d_win;
	int b0, t0, l0, r0;
	double b, t, l, r;

	list = find_item(pad, "list");
	if (!list || !list->list)
	    continue;

	d_win = find_item(pad, "d_win");
	if (!d_win || !d_win->list || !d_win->list->value)
	    continue;

	if (sscanf(d_win->list->value, "%d %d %d %d", &t0, &b0, &l0, &r0) !=
	    4)
	    continue;

	commands = list_copy(list->list, commands);

	sprintf(buff, "d.frame -s %s", pad->name);
	commands = list_push(buff, commands);

	b = 100.0 - 100.0 * b0 / (screen_bottom - screen_top);
	t = 100.0 - 100.0 * t0 / (screen_bottom - screen_top);
	l = 100.0 * l0 / (screen_right - screen_left);
	r = 100.0 * r0 / (screen_right - screen_left);

	sprintf(buff, "d.frame -c %s at=%f,%f,%f,%f", pad->name, b, t, l, r);
	commands = list_push(buff, commands);
    }

    if (!commands)
	return;

    pid = fork();
    if (pid < 0) {
	perror("Monitor: fork");
	return;
    }

    if (pid != 0) {		/* parent */
	set_busy(1);
	redraw_pid = pid;
	list_free(commands);
	return;
    }

    /* child */

    sprintf(buff, "MONITOR_OVERRIDE=%s", monitor_name);
    putenv(buff);

    close(0);
    open("/dev/null", O_RDONLY);
    close(1);
    open("/dev/null", O_WRONLY);
    close(2);
    open("/dev/null", O_WRONLY);
    for (; commands; commands = commands->next)
	system(commands->value);
    exit(0);
}

static void handle_resize_event(void)
{
    PAD *curpad, *nextpad;
    char buf[64];
    XWindowAttributes xwa;
    XGCValues gc_values;

    /* Get the window's current attributes. */
    if (!XGetWindowAttributes(dpy, grwin, &xwa))
	return;

    screen_right = xwa.width;
    screen_bottom = xwa.height;

    /* do a d.frame -e (essentially) */
    /* Dclearscreen() */
    /* delete the time and current window out of the scratch pad */
    curpad = find_pad("");
    delete_item(curpad, "time");
    delete_item(curpad, "cur_w");

    /* delete all other pads */
    for (curpad = pad_list(); curpad != NULL; curpad = nextpad) {
	nextpad = curpad->next;
	if (*curpad->name)
	    delete_pad(curpad);
    }

    curpad = NULL;

    /* Dnew("full_screen") */
    /* find a pad called "full_screen" */
    create_pad("full_screen");
    sprintf(buf, "%d %d %d %d",
	    screen_top, screen_bottom, screen_left, screen_right);
    curpad = find_pad("full_screen");
    append_item(curpad, "d_win", buf, 0);
    _time_stamp(curpad);
    /* Dchoose("full_screen") */

    /* set the time and window name in no-name pad */
    curpad = find_pad("");
    append_item(curpad, "cur_w", "full_screen", 0);
    _time_stamp(curpad);

    /* set the window */
    XD_Set_window(screen_top, screen_bottom, screen_left, screen_right);

    /* Handle backing store */
    XFreePixmap(dpy, bkupmap);
    bkupmap = XCreatePixmap(dpy, grwin, xwa.width, xwa.height, xwa.depth);
    XGetGCValues(dpy, gc, GCForeground, &gc_values);
    XSetForeground(dpy, gc, WhitePixel(dpy, scrn));
    XFillRectangle(dpy, bkupmap, gc, 0, 0, xwa.width, xwa.height);
    XSetForeground(dpy, gc, gc_values.foreground);
    XSetWindowBackgroundPixmap(dpy, grwin, bkupmap);
    XClearWindow(dpy, grwin);

    needs_flush = 0;
}

static void check_flush(void)
{
    static struct timeval last_flush;
    struct timeval now;
    long delta;

    if (!needs_flush)
	return;

    if (gettimeofday(&now, NULL) < 0) {
	perror("Monitor: gettimeofday");
	return;
    }

    delta = (now.tv_sec - last_flush.tv_sec) * 1000000 +
	(now.tv_usec - last_flush.tv_usec);
    if (last_flush.tv_sec && delta < 250000)
	return;

    XClearWindow(dpy, grwin);

    last_flush = now;
    needs_flush = 0;
}

static void set_title_busy(int busy)
{
#ifndef X11R3
    static const char text[] = " [redraw]";
    XTextProperty prop;
    char title[1024], *p;

    if (external_window)
	return;

    if (!XGetWMName(dpy, grwin, &prop)) {
	G_warning(_("Monitor: XGetWMName failed"));
	return;
    }

    if (!prop.value || !prop.nitems || prop.format != 8) {
	G_warning(_("Monitor: XGetWMName: bad result"));
	return;
    }

    strcpy(title, prop.value);
    XFree(prop.value);

    p = strstr(title, text);
    if (p)
	*p = '\0';
    if (busy)
	strcat(title, text);

    prop.value = title;
    prop.nitems = strlen(title);

    XSetWMName(dpy, grwin, &prop);
#endif
}

static void set_busy(int busy)
{
    set_title_busy(busy);

    if (busy)
	XDefineCursor(dpy, grwin, cur_clock);
    else
	XUndefineCursor(dpy, grwin);

    XFlush(dpy);
}
