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

#include <stdlib.h>
#include <wx/button.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/sizer.h>
#include <wx/strconv.h>
#include <wx/event.h>
#include <wx/dcclient.h>
#include <wx/dc.h>
#include <wx/colour.h>
#include "gui.h"

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

enum {
    MYID_REW,
    MYID_RPLAY,
    MYID_STEPB,
    MYID_STOP,
    MYID_STEPF,
    MYID_PLAY,
    MYID_LOOP,
    MYID_SWING,
    MYID_SLOWER,
    MYID_FASTER,
    MYID_SHNAMES,
    MYID_DOEXIT,
};

BEGIN_EVENT_TABLE(MyCanvas, wxPanel)
EVT_ERASE_BACKGROUND(MyCanvas::erase)
END_EVENT_TABLE()

MyCanvas::MyCanvas(wxWindow *parent, wxWindowID id, const wxSize &size)
    : wxPanel(parent, id, wxDefaultPosition, size)
{
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}

void MyCanvas::erase(wxEraseEvent &ev)
{
    ev.GetDC();
}

void MyCanvas::draw_image(wxBitmap *bmp)
{
    wxClientDC dc(this);

    dc.DrawBitmap(*bmp, 0, 0, false);
}

void MyCanvas::draw_text(int style, int x, int y, const char *str)
{
    wxClientDC dc(this);

    switch (style) {
    case 1:
	dc.SetTextBackground(*wxWHITE);
	dc.SetTextForeground(*wxBLACK);
	break;
    case 2:
	dc.SetTextBackground(*wxBLACK);
	dc.SetTextForeground(*wxWHITE);
	break;
    default:
	return;
    }

    dc.DrawText(wxString(str, wxConvISO8859_1), x, y);
}

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_BUTTON(MYID_REW,     MyFrame::rewind_callback)
EVT_BUTTON(MYID_RPLAY,   MyFrame::rplay_callback)
EVT_BUTTON(MYID_STEPB,   MyFrame::stepb_callback)
EVT_BUTTON(MYID_STOP,    MyFrame::stop_callback)
EVT_BUTTON(MYID_STEPF,   MyFrame::stepf_callback)
EVT_BUTTON(MYID_PLAY,    MyFrame::play_callback)
EVT_BUTTON(MYID_LOOP,    MyFrame::loop_callback)
EVT_BUTTON(MYID_SWING,   MyFrame::swing_callback)
EVT_BUTTON(MYID_SLOWER,  MyFrame::slower_callback)
EVT_BUTTON(MYID_FASTER,  MyFrame::faster_callback)
EVT_BUTTON(MYID_SHNAMES, MyFrame::names_callback)
EVT_BUTTON(MYID_DOEXIT,  MyFrame::exit_callback)
END_EVENT_TABLE()

void MyFrame::make_buttons(wxSizer *sizer)
{
    sizer->Add(new wxBitmapButton(this, MYID_REW   , wxBitmap(rewind_bits, rewind_width, rewind_height)));
    sizer->Add(new wxBitmapButton(this, MYID_RPLAY , wxBitmap(rplay_bits , rplay_width , rplay_height )));
    sizer->Add(new wxBitmapButton(this, MYID_STEPB , wxBitmap(stepb_bits , stepb_width , stepb_height )));
    sizer->Add(new wxBitmapButton(this, MYID_STOP  , wxBitmap(stop_bits  , stop_width  , stop_height  )));
    sizer->Add(new wxBitmapButton(this, MYID_STEPF , wxBitmap(stepf_bits , stepf_width , stepf_height )));
    sizer->Add(new wxBitmapButton(this, MYID_PLAY  , wxBitmap(play_bits  , play_width  , play_height  )));
    sizer->Add(new wxBitmapButton(this, MYID_LOOP  , wxBitmap(loop_bits  , loop_width  , loop_height  )));
    sizer->Add(new wxBitmapButton(this, MYID_SWING , wxBitmap(swing_bits , swing_width , swing_height )));
    sizer->Add(new wxBitmapButton(this, MYID_SLOWER, wxBitmap(snail_bits , snail_width , snail_height )));
    sizer->Add(new wxBitmapButton(this, MYID_FASTER, wxBitmap(rabbit_bits, rabbit_width, rabbit_height)));
    sizer->Add(new wxButton(this, MYID_SHNAMES, wxString("Names", wxConvISO8859_1)));
    sizer->Add(new wxButton(this, MYID_DOEXIT,  wxString("Exit", wxConvISO8859_1)));
}

void MyFrame::rewind_callback(wxCommandEvent &event)
{
    cd->step = 0;
    cd->stop = 1;
    cd->rewind = 1;
}

void MyFrame::rplay_callback(wxCommandEvent &event)
{
    cd->step = 0;
    cd->stop = 0;
    cd->direction = -1;
    cd->curframe = cd->prevframe + cd->direction;
}

void MyFrame::stepb_callback(wxCommandEvent &event)
{
    cd->step = 1;
    cd->direction = -1;
    cd->curframe = cd->prevframe + cd->direction;
}

void MyFrame::stop_callback(wxCommandEvent &event)
{
    cd->stop = 1;
}

void MyFrame::stepf_callback(wxCommandEvent &event)
{
    cd->step = 1;
    cd->direction = 1;
    cd->curframe = cd->prevframe + cd->direction;
}

void MyFrame::play_callback(wxCommandEvent &event)
{
    cd->step = 0;
    cd->stop = 0;
    cd->direction = 1;
    cd->curframe = cd->prevframe + cd->direction;
}

void MyFrame::loop_callback(wxCommandEvent &event)
{
    cd->loop = !cd->loop;
    cd->swing = 0;
    cd->stop = !cd->loop;
}

void MyFrame::swing_callback(wxCommandEvent &event)
{
    cd->swing = !cd->swing;
    cd->loop = 0;
    cd->stop = !cd->swing;
}

void MyFrame::slower_callback(wxCommandEvent &event)
{
    if (cd->speed) {
	if (cd->speed < 200000)
	    cd->speed *= 3;
    }
    else
	cd->speed = 1;
}

void MyFrame::faster_callback(wxCommandEvent &event)
{
    if (cd->speed > 1)
	cd->speed /= 3;
}

void MyFrame::names_callback(wxCommandEvent &event)
{
    cd->shownames = (1 + cd->shownames) % 3;
}

void MyFrame::exit_callback(wxCommandEvent &event)
{
    exit(0);
}

void MyFrame::change_label(const char *label)
{
    flabel->SetLabel(wxString(label, wxConvISO8859_1));
}

#if 0

void MyFrame::init_graphics()
{
/* global variables */
    Widget canvas;
    Display *theDisplay;
    XImage *pic_array[MAXIMAGES];
    GC invertGC, drawGC;

    int scrn;
    Display *dpy;
    Window grwin;
    Colormap fixedcmap;
    unsigned long blackPix, whitePix;

    unsigned int depth;
    Visual *use_visual;

    dpy = XtDisplay(canvas);
    grwin = XtWindow(canvas);
    scrn = DefaultScreen(dpy);
    use_visual = DefaultVisual(dpy, scrn);
#if 1
    fixedcmap = XCreateColormap(dpy, grwin, use_visual, AllocNone);
#else
    fixedcmap = DefaultColormap(dpy, scrn);
#endif
    fixedcmap = InitColorTableFixed(fixedcmap);

    XtVaGetValues(canvas, XmNdepth, &depth, NULL);

    XtVaSetValues(toplevel, XmNcolormap, fixedcmap, NULL);
    XtSetWMColormapWindows(toplevel, &canvas, 1);

    /**************************************************************/

    blackPix = _get_lookup_for_color(0, 0, 0);
    whitePix = _get_lookup_for_color(255, 255, 255);

    drawGC =
	XCreateGC(XtDisplay(canvas), XtWindow(canvas), (unsigned long)0,
		  NULL);

    theDisplay = XtDisplay(toplevel);

    XSetFunction(theDisplay, drawGC, GXcopy);
    XSetForeground(theDisplay, drawGC, blackPix);
    XSetBackground(theDisplay, drawGC, whitePix);

    invertGC =
	XCreateGC(XtDisplay(canvas), XtWindow(canvas), (unsigned long)0,
		  NULL);
    XSetFunction(theDisplay, invertGC, GXcopy);
    XSetForeground(theDisplay, invertGC, whitePix);
    XSetBackground(theDisplay, invertGC, blackPix);

}

#endif

MyFrame::MyFrame(const wxString& title, int ncols, int nrows, struct gui_data *cd)
    : wxFrame((wxFrame *)NULL, wxID_ANY, title), cd(cd)
{
    canvas = new MyCanvas(this, wxID_ANY, wxSize(ncols, nrows));

    wxBoxSizer *sizer, *buttons;

    if (ncols > nrows) {
	sizer = new wxBoxSizer(wxVERTICAL);
	buttons = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(canvas);
	sizer->Add(buttons);
    }
    else {
	sizer = new wxBoxSizer(wxHORIZONTAL);
	buttons = new wxBoxSizer(wxVERTICAL);
	sizer->Add(canvas);
	sizer->Add(buttons);
    }

    make_buttons(buttons);

    flabel = new wxStaticText(this, wxID_ANY, wxString("00000", wxConvISO8859_1), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    buttons->Add(flabel);

    SetSizerAndFit(sizer);
}

