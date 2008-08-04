#include "defs.h"


/*  #define DEBUG2  1  */

#ifndef GLOBAL
#  define GLOBAL extern
#  define INIT(x)
#else
#  define INIT(x) = x
#endif

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

GLOBAL View *VIEW_ELEV;
GLOBAL View *VIEW_TITLE_ELEV;

GLOBAL void *elevbuf;
GLOBAL int elev;
GLOBAL char *elev_layer;
GLOBAL char *mapset_elev;

GLOBAL View *VIEW_MENU;

GLOBAL struct Ortho_Image_Group group;
GLOBAL struct Ortho_Control_Points temp_points;

GLOBAL char interrupt_char;
GLOBAL char *tempfile1;
GLOBAL char *tempfile2;
GLOBAL char *tempfile_dot;
GLOBAL char *tempfile_dot2;
GLOBAL char *tempfile_win;
GLOBAL char *tempfile_win2;
GLOBAL char *tempfile_elev;
GLOBAL char *digit_points;	/* digitizer control points */
GLOBAL int use_digitizer INIT(0);	/* is there a digitizer out there? */

/* group file list, target cell,vector files */
GLOBAL char *group_list INIT(NULL);
GLOBAL char *cell_list INIT(NULL);
GLOBAL char *vect_list INIT(NULL);

GLOBAL int from_keyboard INIT(-1);	/* input method */
GLOBAL int from_digitizer INIT(-1);
GLOBAL int from_screen INIT(-1);
GLOBAL int from_flag INIT(0);

GLOBAL int autozoom_flag INIT(1);
GLOBAL int autozoom_on INIT(1);
GLOBAL int autozoom_off INIT(0);

GLOBAL int dotsize INIT(4);


GLOBAL int COLOR[10];

#define BLACK	COLOR[0]
#define BLUE	COLOR[1]
#define BROWN	COLOR[2]
#define GREEN	COLOR[3]
#define GREY	COLOR[4]
#define ORANGE	COLOR[5]
#define PURPLE	COLOR[6]
#define RED	COLOR[7]
#define WHITE	COLOR[8]
#define YELLOW	COLOR[9]

#undef INIT
