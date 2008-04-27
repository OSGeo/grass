#include <grass/colors.h>
#include "defs.h"

#ifndef GLOBAL
#  define GLOBAL extern
#  define INIT(x)
#else
#  define INIT(x) = x
#endif

GLOBAL int cellmap_present;

GLOBAL int SCREEN_TOP;
GLOBAL int SCREEN_BOTTOM;
GLOBAL int SCREEN_LEFT;
GLOBAL int SCREEN_RIGHT;

GLOBAL Window *INFO_WINDOW;
GLOBAL Window *MENU_WINDOW;
GLOBAL Window *PROMPT_WINDOW;

GLOBAL View *VIEW_MAP1;
GLOBAL View *VIEW_TITLE1;
GLOBAL View *VIEW_MAP1_ZOOM;
GLOBAL View *VIEW_TITLE1_ZOOM;

GLOBAL View *VIEW_MAP2;
GLOBAL View *VIEW_TITLE2;
GLOBAL View *VIEW_MAP2_ZOOM;
GLOBAL View *VIEW_TITLE2_ZOOM;

GLOBAL View *VIEW_MENU;

GLOBAL Group group;

GLOBAL char interrupt_char;
GLOBAL char *tempfile1;
GLOBAL char *tempfile2;
GLOBAL char *digit_points;            /* digitizer control points */
GLOBAL char *digit_results;           /* digitizer results */
GLOBAL int  use_digitizer INIT(0);    /* is there a digitizer out there? */

/* group file list, target cell,vector files */
GLOBAL char *group_list INIT(NULL);
GLOBAL char *cell_list INIT(NULL);
GLOBAL char *vect_list INIT(NULL);

GLOBAL int  from_keyboard  INIT(-1);   /* input method */
GLOBAL int  from_digitizer INIT(-1);
GLOBAL int  from_screen    INIT(-1);
GLOBAL int  from_flag      INIT(0);

GLOBAL int  dotsize INIT(4);

GLOBAL int line_color;

/* analyze.c */
int delete_control_point(int);
int analyze(void);
/* ask.c */
int ask_gis_files(char *, char *, char *, char *, int);
/* ask_lineclr.c */
int ask_line_color(char *, int, char *, int);
/* ask_mag.c */
int ask_magnification(int *);
int draw_mag(void);
/* call.c */
int call(int (*)(void), char *);
/* cell.c */
int plotcell(int, int);
/* cellhd.c */
int Outline_cellhd(View *, struct Cell_head *);
/* colors.c */
int set_colors(struct Colors *);
/*
 int set_menu_colors(struct Colors *);
 int cursor_color(void);
*/
int get_vector_color(void);
/* conv.c */
int view_to_col(View *, int);
int view_to_row(View *, int);
int col_to_view(View *, int);
int row_to_view(View *, int);
double row_to_northing(struct Cell_head *, int, double);
double col_to_easting(struct Cell_head *, int, double);
double northing_to_row(struct Cell_head *, double);
double easting_to_col(struct Cell_head *, double);
/* crs.c */
int CRS_georef(double, double, double *, double *, double [], double [], int);
int CRS_compute_georef_equations(struct Control_Points *, double [], double [], double [], double [], int);
/* curses.c */
int Begin_curses(void);
int End_curses(void);
int Suspend_curses(void);
int Resume_curses(void);
int Curses_allow_interrupts(int);
int Curses_clear_window(Window *);
int Curses_outline_window(Window *);
int Curses_write_window(Window *, int, int, char *);
int Curses_replot_screen(void);
int Curses_prompt_gets(char *, char *);
int Beep(void);
int Curses_getch(int);
/* debug.c */
int debug(char *,...);
/* digit.c */
int setup_digitizer(void);
int digitizer_point(double *, double *);
/* dot.c */
int dot(int, int);
int save_under_dot(int, int);
int restore_under_dot(void);
int release_under_dot(void);
/* drawcell.c */
int drawcell(View *, int);
int re_fresh_rast(void);
/* drawvect.c */
int plotvect(void);
int zoomvect(View *);
int re_fresh_vect(void);
int warpvect(double [], double [], int);
/* driver.c */
int driver(void);
/* equ.c */
int CRS_Compute_equation(int);
/* find.c */
int find_target_files(void);
/* graphics.c */
int Init_graphics(void);
int Outline_box(int, int, int, int);
int Text_width(char *);
int Text(char *, int, int, int, int, int);
int Uparrow(int, int, int, int);
int Downarrow(int, int, int, int);
/* group.c */
int prepare_group_list(void);
int choose_groupfile(char *, char *);
/* input.c */
int Input_pointer(Objects *);
int Input_box(Objects *, int, int);
int Input_other(int (*)(void), char *);
int Menu_msg(char *);
int Start_mouse_in_menu(void);
/* main.c */
int main(int, char *[]);
#ifdef __GNUC_MINOR__
int quit(int) __attribute__ ((__noreturn__));
#else
int quit(int);
#endif
int error(const char *, int);
/* mark.c */
int mark(int, int, int);
int mark_point(View *, int, int);
/* mouse.c */
int Mouse_pointer(int *, int *, int *);
int Mouse_box_anchored(int, int, int *, int *, int *);
int Get_mouse_xy(int *, int *);
int Set_mouse_xy(int, int);
/* open.c */
FILE *open_vect(char *, char *);
int close_vect(FILE *);
/* points.c */
int display_points(int);
int display_points_in_view(View *, int, double *, double *, int *, int);
int display_one_point(View *, double, double);
/* setup.c */
int dsp_setup(int, struct Cell_head *);
/* target.c */
int get_target(void);
int select_current_env(void);
int select_target_env(void);
/* title.c */
int display_title(View *);
/* view.c */
int Configure_view(View *, char *, char *, double, double);
int In_view(View *, int, int);
int Erase_view(View *);
double magnification(View *);
/* where.c */
int where(int, int);
/* zoom.c */
int zoom(void);
/* zoom_box.c */
int zoom_box(void);
/* zoom_pnt.c */
int zoom_point(void);

#undef INIT
