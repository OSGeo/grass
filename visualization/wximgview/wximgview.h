
// -*- c++ -*-

#include <wx/wx.h>

#ifndef WXIMGVIEW_H_
#define WXIMGVIEW_H_

class MyFrame: public wxFrame
{
private:
    wxTimer *timer;

    void draw();

    static const wxString title;

public:
    MyFrame(const wxSize &size)
	: wxFrame((wxFrame *)0, wxID_ANY, title, wxDefaultPosition, size)
    { Create(); }

    void redraw(wxPaintEvent &ev);
    void erase(wxEraseEvent &ev);
    void tick(wxTimerEvent &ev);

private:
    void Create();

    DECLARE_EVENT_TABLE()
};

class MyApp: public wxApp
{
public:
    virtual bool OnInit();

public:
    MyFrame *mainwin;
    int i_width, i_height;
    unsigned char *imgbuf;
    double last;

    int read_bmp_header(const unsigned char *p);
    void map_file();
};

#endif // WXIMGVIEW_H_
