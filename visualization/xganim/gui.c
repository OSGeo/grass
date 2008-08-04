/* Written by Bill Brown, USACERL (brown@zorro.cecer.army.mil)
 * May 2-12, 1994
 *
 * This code is in the public domain. Specifically, we give to the public
 * domain all rights for future licensing of the source code, all resale
 * rights, and all publishing rights.
 * 
 * We ask, but do not require, that the following message be included in
 * all derived works:
 *     "Portions developed at the US Army Construction Engineering 
 *     Research Laboratories, Champaign, Illinois."
 * 
 * USACERL GIVES NO WARRANTY, EXPRESSED OR IMPLIED,
 * FOR THE SOFTWARE AND/OR DOCUMENTATION PROVIDED, INCLUDING, WITHOUT
 * LIMITATION, WARRANTY OF MERCHANTABILITY AND WARRANTY OF FITNESS FOR A
 * PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include "bitmaps/rewind.xbm"
#include "bitmaps/rplay.xbm"
#include "bitmaps/stepb.xbm"
#include "bitmaps/stop.xbm"
#include "bitmaps/stepf.xbm"
#include "bitmaps/play.xbm"
#include "bitmaps/loop.xbm"
#include "bitmaps/swing.xbm"
#include "bitmaps/snail.xbm"
#include "bitmaps/rabbit.xbm"

#include "gui.h"


/* callback procs */
static void rewind_callback(Widget w, XtPointer data, caddr_t cbs);
static void rplay_callback(Widget w, XtPointer data, caddr_t cbs);
static void stepb_callback(Widget w, XtPointer data, caddr_t cbs);
static void stop_callback(Widget w, XtPointer data, caddr_t cbs);
static void stepf_callback(Widget w, XtPointer data, caddr_t cbs);
static void play_callback(Widget w, XtPointer data, caddr_t cbs);
static void loop_callback(Widget w, XtPointer data, caddr_t cbs);
static void swing_callback(Widget w, XtPointer data, caddr_t cbs);
static void exit_callback(Widget w, XtPointer data, caddr_t cbs);
static void names_callback(Widget w, XtPointer data, caddr_t cbs);
static void slower_callback(Widget w, XtPointer data, caddr_t cbs);
static void faster_callback(Widget w, XtPointer data, caddr_t cbs);

/* global variables */
static Widget rew, rplay, stepb, stop, stepf, play, loop, swing;
static Widget slower, faster;
static struct gui_data *cd;


void make_buttons(struct gui_data *data, Widget trc, Screen * scr)
{
    unsigned int n;
    Arg wargs[10];
    Widget shnames, doexit;

    cd = data;

    /***************** rewind */
    n = 0;
    XtSetArg(wargs[n], XmNforeground, BlackPixelOfScreen(scr));
    n++;
    XtSetArg(wargs[n], XmNheight, 30);
    n++;
    XtSetArg(wargs[n], XmNwidth, 30);
    n++;
    rew = XtCreateManagedWidget("rd", xmPushButtonWidgetClass, trc, wargs, n);

    /***************** rplay */
    n = 0;
    XtSetArg(wargs[n], XmNforeground, BlackPixelOfScreen(scr));
    n++;
    XtSetArg(wargs[n], XmNheight, 30);
    n++;
    XtSetArg(wargs[n], XmNwidth, 30);
    n++;
    rplay = XtCreateManagedWidget("ry", xmPushButtonWidgetClass,
				  trc, wargs, n);

    /***************** stepb */
    n = 0;
    XtSetArg(wargs[n], XmNforeground, BlackPixelOfScreen(scr));
    n++;
    XtSetArg(wargs[n], XmNheight, 30);
    n++;
    XtSetArg(wargs[n], XmNwidth, 30);
    n++;
    stepb = XtCreateManagedWidget("sb", xmPushButtonWidgetClass,
				  trc, wargs, n);

    /***************** stop */
    n = 0;
    XtSetArg(wargs[n], XmNforeground, BlackPixelOfScreen(scr));
    n++;
    XtSetArg(wargs[n], XmNheight, 30);
    n++;
    XtSetArg(wargs[n], XmNwidth, 30);
    n++;
    stop = XtCreateManagedWidget("sp", xmPushButtonWidgetClass,
				 trc, wargs, n);

    /***************** stepf */
    n = 0;
    XtSetArg(wargs[n], XmNforeground, BlackPixelOfScreen(scr));
    n++;
    XtSetArg(wargs[n], XmNheight, 30);
    n++;
    XtSetArg(wargs[n], XmNwidth, 30);
    n++;
    stepf = XtCreateManagedWidget("sf", xmPushButtonWidgetClass,
				  trc, wargs, n);

    /***************** play */
    n = 0;
    XtSetArg(wargs[n], XmNforeground, BlackPixelOfScreen(scr));
    n++;
    XtSetArg(wargs[n], XmNheight, 30);
    n++;
    XtSetArg(wargs[n], XmNwidth, 30);
    n++;
    play = XtCreateManagedWidget("py", xmPushButtonWidgetClass,
				 trc, wargs, n);

    /***************** loop */
    n = 0;
    XtSetArg(wargs[n], XmNforeground, BlackPixelOfScreen(scr));
    n++;
    XtSetArg(wargs[n], XmNheight, 30);
    n++;
    XtSetArg(wargs[n], XmNwidth, 30);
    n++;
    loop = XtCreateManagedWidget("lp", xmToggleButtonWidgetClass,
				 trc, wargs, n);

    /***************** swing */
    n = 0;
    XtSetArg(wargs[n], XmNforeground, BlackPixelOfScreen(scr));
    n++;
    XtSetArg(wargs[n], XmNheight, 30);
    n++;
    XtSetArg(wargs[n], XmNwidth, 30);
    n++;
    swing = XtCreateManagedWidget("sw", xmToggleButtonWidgetClass,
				  trc, wargs, n);

    /***************** slower */
    n = 0;
    XtSetArg(wargs[n], XmNforeground, BlackPixelOfScreen(scr));
    n++;
    XtSetArg(wargs[n], XmNheight, 30);
    n++;
    XtSetArg(wargs[n], XmNwidth, 30);
    n++;
    slower = XtCreateManagedWidget("Sl", xmPushButtonWidgetClass,
				   trc, wargs, n);

    /***************** faster */
    n = 0;
    XtSetArg(wargs[n], XmNforeground, BlackPixelOfScreen(scr));
    n++;
    XtSetArg(wargs[n], XmNheight, 30);
    n++;
    XtSetArg(wargs[n], XmNwidth, 30);
    n++;
    faster = XtCreateManagedWidget("Fa", xmPushButtonWidgetClass,
				   trc, wargs, n);

    /***************** shownames */
    n = 0;
    XtSetArg(wargs[n], XmNforeground, BlackPixelOfScreen(scr));
    n++;
    XtSetArg(wargs[n], XmNheight, 30);
    n++;
    XtSetArg(wargs[n], XmNwidth, 30);
    n++;
    XtSetArg(wargs[n], XmNset, TRUE);
    n++;
    shnames = XtCreateManagedWidget("names", xmPushButtonWidgetClass,
				    trc, wargs, n);
    XtAddCallback(shnames, XmNactivateCallback,
		  (XtCallbackProc) names_callback, (XtPointer) NULL);

    n = 0;
    XtSetArg(wargs[n], XmNmarginLeft, 0);
    n++;
    doexit = XtCreateManagedWidget("Exit", xmPushButtonWidgetClass,
				   trc, wargs, n);
    XtAddCallback(doexit, XmNactivateCallback, (XtCallbackProc) exit_callback,
		  (XtPointer) NULL);
}

void set_buttons_pixmap(Display * display, Drawable d)
{
    Pixel fg, bg;
    unsigned int depth;
    Pixmap button_pix;

    XtVaGetValues(rew, XmNforeground, &fg,
		  XmNbackground, &bg, XmNdepth, &depth, NULL);
    button_pix = XCreatePixmapFromBitmapData(display,
					     d, rewind_bits, rewind_width,
					     rewind_height, fg, bg, depth);
    XtVaSetValues(rew, XmNlabelType, XmPIXMAP, XmNlabelPixmap, button_pix,
		  NULL);
    XtAddCallback(rew, XmNactivateCallback, (XtCallbackProc) rewind_callback,
		  (XtPointer) NULL);

    XtVaGetValues(rplay, XmNforeground, &fg,
		  XmNbackground, &bg, XmNdepth, &depth, NULL);
    button_pix = XCreatePixmapFromBitmapData(display,
					     d, rplay_bits, rplay_width,
					     rplay_height, fg, bg, depth);
    XtVaSetValues(rplay, XmNlabelType, XmPIXMAP, XmNlabelPixmap, button_pix,
		  NULL);
    XtAddCallback(rplay, XmNactivateCallback, (XtCallbackProc) rplay_callback,
		  (XtPointer) NULL);

    XtVaGetValues(stepb, XmNforeground, &fg,
		  XmNbackground, &bg, XmNdepth, &depth, NULL);
    button_pix = XCreatePixmapFromBitmapData(display,
					     d, stepb_bits, stepb_width,
					     stepb_height, fg, bg, depth);
    XtVaSetValues(stepb, XmNlabelType, XmPIXMAP, XmNlabelPixmap, button_pix,
		  NULL);
    XtAddCallback(stepb, XmNactivateCallback, (XtCallbackProc) stepb_callback,
		  (XtPointer) NULL);

    XtVaGetValues(stop, XmNforeground, &fg,
		  XmNbackground, &bg, XmNdepth, &depth, NULL);
    button_pix = XCreatePixmapFromBitmapData(display,
					     d, stop_bits, stop_width,
					     stop_height, fg, bg, depth);
    XtVaSetValues(stop, XmNlabelType, XmPIXMAP, XmNlabelPixmap, button_pix,
		  NULL);
    XtAddCallback(stop, XmNactivateCallback, (XtCallbackProc) stop_callback,
		  (XtPointer) NULL);

    XtVaGetValues(stepf, XmNforeground, &fg,
		  XmNbackground, &bg, XmNdepth, &depth, NULL);
    button_pix = XCreatePixmapFromBitmapData(display,
					     d, stepf_bits, stepf_width,
					     stepf_height, fg, bg, depth);
    XtVaSetValues(stepf, XmNlabelType, XmPIXMAP, XmNlabelPixmap, button_pix,
		  NULL);
    XtAddCallback(stepf, XmNactivateCallback, (XtCallbackProc) stepf_callback,
		  (XtPointer) NULL);

    XtVaGetValues(play, XmNforeground, &fg,
		  XmNbackground, &bg, XmNdepth, &depth, NULL);
    button_pix = XCreatePixmapFromBitmapData(display,
					     d, play_bits, play_width,
					     play_height, fg, bg, depth);
    XtVaSetValues(play, XmNlabelType, XmPIXMAP, XmNlabelPixmap, button_pix,
		  NULL);
    XtAddCallback(play, XmNactivateCallback, (XtCallbackProc) play_callback,
		  (XtPointer) NULL);

    XtVaGetValues(loop, XmNforeground, &fg,
		  XmNbackground, &bg, XmNdepth, &depth, NULL);
    button_pix = XCreatePixmapFromBitmapData(display,
					     d, loop_bits, loop_width,
					     loop_height, fg, bg, depth);
    XtVaSetValues(loop, XmNlabelType, XmPIXMAP, XmNlabelPixmap, button_pix,
		  NULL);
    XtAddCallback(loop, XmNvalueChangedCallback,
		  (XtCallbackProc) loop_callback, (XtPointer) NULL);

    XtVaGetValues(swing, XmNforeground, &fg,
		  XmNbackground, &bg, XmNdepth, &depth, NULL);
    button_pix = XCreatePixmapFromBitmapData(display,
					     d, swing_bits, swing_width,
					     swing_height, fg, bg, depth);
    XtVaSetValues(swing, XmNlabelType, XmPIXMAP, XmNlabelPixmap, button_pix,
		  NULL);
    XtAddCallback(swing, XmNvalueChangedCallback,
		  (XtCallbackProc) swing_callback, (XtPointer) NULL);

    XtVaGetValues(faster, XmNforeground, &fg,
		  XmNbackground, &bg, XmNdepth, &depth, NULL);
    button_pix = XCreatePixmapFromBitmapData(display,
					     d, rabbit_bits, rabbit_width,
					     rabbit_height, fg, bg, depth);
    XtVaSetValues(faster, XmNlabelType, XmPIXMAP, XmNlabelPixmap, button_pix,
		  NULL);
    XtAddCallback(faster, XmNactivateCallback,
		  (XtCallbackProc) faster_callback, (XtPointer) NULL);

    XtVaGetValues(slower, XmNforeground, &fg,
		  XmNbackground, &bg, XmNdepth, &depth, NULL);
    button_pix = XCreatePixmapFromBitmapData(display,
					     d, snail_bits, snail_width,
					     snail_height, fg, bg, depth);
    XtVaSetValues(slower, XmNlabelType, XmPIXMAP, XmNlabelPixmap, button_pix,
		  NULL);
    XtAddCallback(slower, XmNactivateCallback,
		  (XtCallbackProc) slower_callback, (XtPointer) NULL);

}

static void rewind_callback(Widget w, XtPointer data, caddr_t cbs)
{

    cd->step = 0;
    cd->stop = 1;
    cd->rewind = 1;

}

static void rplay_callback(Widget w, XtPointer data, caddr_t cbs)
{
    cd->step = 0;
    cd->stop = 0;
    cd->direction = -1;
    cd->curframe = cd->prevframe + cd->direction;

}

static void stepb_callback(Widget w, XtPointer data, caddr_t cbs)
{
    cd->step = 1;
    cd->direction = -1;
    cd->curframe = cd->prevframe + cd->direction;
}

static void stop_callback(Widget w, XtPointer data, caddr_t cbs)
{
    cd->stop = 1;
}

static void stepf_callback(Widget w, XtPointer data, caddr_t cbs)
{
    cd->step = 1;
    cd->direction = 1;
    cd->curframe = cd->prevframe + cd->direction;
}

static void play_callback(Widget w, XtPointer data, caddr_t cbs)
{
    cd->step = 0;
    cd->stop = 0;
    cd->direction = 1;
    cd->curframe = cd->prevframe + cd->direction;
}

static void loop_callback(Widget w, XtPointer data, caddr_t cbs)
{
    cd->loop = XmToggleButtonGetState(loop);
    cd->swing = 0;
    XmToggleButtonSetState(swing, 0, False);
    cd->stop = !cd->loop;
}

static void swing_callback(Widget w, XtPointer data, caddr_t cbs)
{
    cd->swing = XmToggleButtonGetState(swing);
    cd->loop = 0;
    XmToggleButtonSetState(loop, 0, False);
    cd->stop = !cd->swing;
}

static void slower_callback(Widget w, XtPointer data, caddr_t cbs)
{
    if (cd->speed) {
	if (cd->speed < 200000)
	    cd->speed *= 3;
    }
    else
	cd->speed = 1;
}

static void faster_callback(Widget w, XtPointer data, caddr_t cbs)
{
    if (cd->speed > 1)
	cd->speed /= 3;
}

static void names_callback(Widget w, XtPointer data, caddr_t cbs)
{
    cd->shownames = (1 + cd->shownames) % 3;
}

static void exit_callback(Widget w, XtPointer data, caddr_t cbs)
{
    exit(0);
}
