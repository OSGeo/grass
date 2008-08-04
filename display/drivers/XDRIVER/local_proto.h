
/* Clr_table.c */
Colormap init_color_table(Colormap cmap);

/* Serve_Xevent.c */
int get_xevent(long event_mask, XEvent * event);
int service_xevent(int);

/* alloc.c */
XPoint *alloc_xpoints(int);
