#include <wx/app.h>
#include <wx/event.h>
#include <wx/window.h>
#include <wx/panel.h>
#include <wx/frame.h>
#include <wx/stattext.h>
#include <wx/bitmap.h>

#define MAXIMAGES 400
#define MAXVIEWS    4

struct gui_data
{
    int step, speed, stop, direction;
    int rewind, prevframe, curframe, nframes;
    int loop, swing, shownames;
};

class MyCanvas: public wxPanel
{
public:
    MyCanvas(wxWindow *parent, wxWindowID id, const wxSize& size);
    void erase(wxEraseEvent &ev);
    void draw_image(wxBitmap *bmp);
    void draw_text(int style, int x, int y, const char *str);

    DECLARE_EVENT_TABLE()
};

class MyFrame: public wxFrame
{
public:
    MyFrame(const wxString& title, int ncols, int nrows, struct gui_data *cd);
    void change_label(const char *label);

    MyCanvas *canvas;

private:
    struct gui_data *cd;
    wxStaticText *flabel;

    void make_buttons(wxSizer *);

    void rewind_callback(wxCommandEvent &);
    void rplay_callback(wxCommandEvent &);
    void stepb_callback(wxCommandEvent &);
    void stop_callback(wxCommandEvent &);
    void stepf_callback(wxCommandEvent &);
    void play_callback(wxCommandEvent &);
    void loop_callback(wxCommandEvent &);
    void swing_callback(wxCommandEvent &);
    void exit_callback(wxCommandEvent &);
    void names_callback(wxCommandEvent &);
    void slower_callback(wxCommandEvent &);
    void faster_callback(wxCommandEvent &);

    DECLARE_EVENT_TABLE()
};

class MyApp: public wxApp
{
private:
    wxBitmap *pic_array[MAXIMAGES];

public:
    virtual bool OnInit();

private:
    MyFrame *mainwin;
    struct gui_data gd;

    int load_files(void);
    void do_run(wxIdleEvent &);

    DECLARE_EVENT_TABLE()
};
