#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/ArrowB.h>

struct gui_data
{
    int step, speed, stop, direction;
    int rewind, prevframe, curframe, nframes;
    int loop, swing, shownames;
};


/* function prototypes */
/* gui.c */
void make_buttons(struct gui_data *data, Widget trc, Screen * scr);
void set_buttons_pixmap(Display * display, Drawable d);

/* Clr_table.c */
Colormap InitColorTableFixed(Colormap cmap);
int _get_lookup_for_color(unsigned int r, unsigned int g, unsigned int b);
