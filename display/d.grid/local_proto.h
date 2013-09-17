#include <grass/gprojects.h>

#define DIRN_BOTH 0 
#define DIRN_LAT  1
#define DIRN_LON  2

/* plot.c */
int plot_grid(double, double, double, int, int, int, int, int, double, int);
int plot_geogrid(double, struct pj_info, struct pj_info, int, int, int, int,
		 int, double, int);
void init_proj(struct pj_info *, struct pj_info *, int);
void get_ll_bounds(double *, double *, double *, double *, struct Cell_head,
		   struct pj_info, struct pj_info);
void check_coords(double, double, double *, double *, int, struct Cell_head,
		  struct pj_info, struct pj_info);
float get_heading(double, double);

/* plotborder.c */
int plot_border(double, double, double, int);

#define MARK_GRID     0
#define MARK_CROSS    1
#define MARK_FIDUCIAL 2
#define MARK_DOT      3

/* fiducial.c */
void plot_cross(double, double, int, double);
void plot_fiducial(double, double, int, double);
void plot_symbol(double, double, int, double, char *, int);
void plot_dot(double, double, int);
