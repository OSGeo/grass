/* variables used within XDRIVER */

extern const char *monitor_name;

extern Display *dpy;
extern Window grwin;

extern Visual *use_visual;
extern int use_bit_depth;

extern int scrn;
extern GC gc;
extern Colormap fixedcmap;
extern Cursor cur_xh, cur_clock;
extern u_long gemask;
extern Pixmap bkupmap;
extern int truecolor;

extern int external_window;

extern unsigned long *xpixels;

extern int needs_flush;

extern pid_t redraw_pid;

extern int current_color;
