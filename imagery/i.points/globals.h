#include "defs.h"

#ifndef GLOBAL
#  define GLOBAL extern
#  define INIT(x)
#else
#  define INIT(x) = x
#endif

GLOBAL int G_get_color();

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


GLOBAL int THE_COLORS[10];
#define BLACK	THE_COLORS[0]
#define BLUE	THE_COLORS[1]
#define BROWN	THE_COLORS[2]
#define GREEN	THE_COLORS[3]
#define GREY	THE_COLORS[4]
#define ORANGE	THE_COLORS[5]
#define PURPLE	THE_COLORS[6]
#define RED	THE_COLORS[7]
#define WHITE	THE_COLORS[8]
#define YELLOW	THE_COLORS[9]

double row_to_northing();
double col_to_easting();
double northing_to_row();
double easting_to_col();

#undef INIT
