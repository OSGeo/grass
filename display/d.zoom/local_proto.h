#include <grass/gis.h>
#include <grass/config.h>

/* box.c */
int make_window_box(struct Cell_head *, double, int, int);

/* center.c */
int make_window_center(struct Cell_head *, double, double, double);

/* returns.c */
int get_wind_bot(void);
int get_wind_top(void);
int get_wind_rite(void);
int get_wind_left(void);
int get_map_bot(void);
int get_map_top(void);
int get_map_left(void);
int get_map_rite(void);
int get_wind_y_pos(float);
int get_wind_x_pos(float);

/* zoom.c */
int zoomwindow(struct Cell_head *, int, double);

/* pan.c */
int do_pan(struct Cell_head *);
int pan_window(struct Cell_head *, int, int);

/* redraw.c */
int redraw(void);

/* print.c */
int print_coor(struct Cell_head *, double, double);
int print_win(struct Cell_head *, double, double, double, double);
int print_limit(struct Cell_head *, struct Cell_head *);

/* set.c */
int set_win(struct Cell_head *, double, double, double, double, int);

/* quit.c */
int quit(struct Cell_head *, struct Cell_head *);

#ifdef MAIN
#define	GLOBAL
#else
#define	GLOBAL	extern
#endif

GLOBAL char *cmd;
GLOBAL char **rast, **vect, **list;
GLOBAL int nrasts, nvects, nlists;
GLOBAL double U_east, U_west, U_south, U_north;
