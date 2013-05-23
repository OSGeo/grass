
/****************************************************************************
 *
 * MODULE:       wximgview
 * AUTHOR(S):    Glynn Clements
 * PURPOSE:      View BMP images from the PNG/cairo drivers
 * COPYRIGHT:    (C) 2010 Glynn Clements
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

#ifdef __MINGW32__
#include <Windows.h>
#else
#include <sys/mman.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <wx/wx.h>

extern "C" {
#include <grass/gis.h>
#include <grass/glocale.h>
}

#include "wximgview.h"

#define HEADER_SIZE 64

IMPLEMENT_APP_NO_MAIN(MyApp)

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_ERASE_BACKGROUND(MyFrame::erase)
EVT_PAINT(MyFrame::redraw)
EVT_TIMER(wxID_ANY, MyFrame::tick)
END_EVENT_TABLE()
    ;
const wxString MyFrame::title = wxString::FromAscii("Image Viewer");

static const char *filename;
static double fraction;

void MyFrame::Create()
{
    timer = new wxTimer(this);
    timer->Start(100, true);
}

void MyFrame::erase(wxEraseEvent &ev)
{
    ev.GetDC();
}

void MyFrame::draw()
{
    MyApp &app = wxGetApp();
    wxSize size = GetSize();
    int x0 = (size.GetWidth()  - app.i_width) / 2;
    int y0 = (size.GetHeight() - app.i_height) / 2;
    wxPaintDC dc(this);
    wxImage image(app.i_width, app.i_height);

    const unsigned char *p = app.imgbuf;
    for (int y = 0; y < app.i_height; y++)
	for (int x = 0; x < app.i_width; x++, p += 4)
	    image.SetRGB(x, y, p[2], p[1], p[0]);

    dc.DrawBitmap(wxBitmap(image), x0, y0, false);
}

void MyFrame::redraw(wxPaintEvent &ev)
{
    if (::fraction > 0.001) {
	struct timeval tv0;
	gettimeofday(&tv0, NULL);

	draw();

	struct timeval tv1;
	gettimeofday(&tv1, NULL);

	double last = (tv1.tv_sec - tv0.tv_sec) * 1e3 + (tv1.tv_usec - tv0.tv_usec) * 1e-3;
	double delay = last / ::fraction;
	timer->Start((int) delay, true);
    }
    else
	draw();
}

void MyFrame::tick(wxTimerEvent &ev)
{
    Refresh();
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

int MyApp::read_bmp_header(const unsigned char *p)
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
    if (get_4(&p) != (unsigned int) i_width * i_height * 4)
	return 0;

    if (size != HEADER_SIZE + i_width * i_height * 4)
	return 0;

    get_4(&p);
    get_4(&p);
    get_4(&p);
    get_4(&p);

    return 1;
}

void MyApp::map_file()
{
    unsigned char header[HEADER_SIZE];
    size_t size;
    void *ptr;
    int fd;

    fd = open(::filename, O_RDONLY);
    if (fd < 0)
	G_fatal_error(_("Unable to open image file"));

    if (read(fd, (char *) header, sizeof(header)) != sizeof(header))
	G_fatal_error(_("Unable to read BMP header"));

    if (!read_bmp_header(header))
	G_fatal_error(_("Invalid BMP header"));

    size = HEADER_SIZE + i_width * i_height * 4;

#ifdef __MINGW32__
    HANDLE handle = CreateFileMapping((HANDLE) _get_osfhandle(fd),
				      NULL, PAGE_READONLY,
				      0, size, NULL);
    if (!handle)
	return;
    ptr = MapViewOfFile(handle, FILE_MAP_READ, 0, 0, size);
    if (!ptr)
	G_fatal_error(_("Unable to map image file"));
#else
    ptr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, (off_t) 0);
    if (ptr == MAP_FAILED)
	G_fatal_error(_("Unable to map image file"));
#endif

    imgbuf = (unsigned char *)ptr + HEADER_SIZE;

    close(fd);
}

static void dummy_handler(int sig)
{
    wxTimerEvent ev = wxTimerEvent();
    wxPostEvent(wxGetApp().mainwin, ev);
}

static void set_handler(void)
{
#ifndef __MINGW32__
    struct sigaction act;

    act.sa_handler = &dummy_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, NULL);
#endif
}

bool MyApp::OnInit()
{
    map_file();

    wxSize size(i_width, i_height);
    mainwin = new MyFrame(size);
    mainwin->Show();
    SetTopWindow(mainwin);

    set_handler();

    return true;
}

int main(int argc, char **argv)
{
    struct GModule *module;
    struct
    {
	struct Option *image, *percent;
    } opt;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("graphics"));
    G_add_keyword(_("raster"));
    G_add_keyword(_("vector"));
    G_add_keyword(_("visualization"));
    module->description = _("View BMP images from the PNG driver.");

    opt.image = G_define_standard_option(G_OPT_F_INPUT);
    opt.image->key = "image";
    opt.image->required = YES;
    opt.image->gisprompt = "old_file,file,file";
    opt.image->description = _("Image file");

    opt.percent = G_define_option();
    opt.percent->key = "percent";
    opt.percent->type = TYPE_INTEGER;
    opt.percent->required = NO;
    opt.percent->multiple = NO;
    opt.percent->description = _("Percentage of CPU time to use");
    opt.percent->answer = "10";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    ::filename = opt.image->answer;
    ::fraction = atoi(opt.percent->answer) / 100.0;

    return wxEntry(argc, argv);

    return EXIT_SUCCESS;
}

