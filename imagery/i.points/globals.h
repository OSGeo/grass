#include "defs.h"

extern int SCREEN_TOP;
extern int SCREEN_BOTTOM;
extern int SCREEN_LEFT;
extern int SCREEN_RIGHT;

extern Window *INFO_WINDOW;
extern Window *MENU_WINDOW;
extern Window *PROMPT_WINDOW;

extern View *VIEW_MAP1;
extern View *VIEW_TITLE1;
extern View *VIEW_MAP1_ZOOM;
extern View *VIEW_TITLE1_ZOOM;

extern View *VIEW_MAP2;
extern View *VIEW_TITLE2;
extern View *VIEW_MAP2_ZOOM;
extern View *VIEW_TITLE2_ZOOM;

extern View *VIEW_MENU;

extern Group group;

extern char interrupt_char;
extern char *tempfile1;
extern char *tempfile2;
extern char *digit_points;	/* digitizer control points */
extern char *digit_results;	/* digitizer results */
extern int use_digitizer;	/* is there a digitizer out there? */

/* group file list, target cell,vector files */
extern char *group_list;
extern char *cell_list;
extern char *vect_list;

extern int from_keyboard;	/* input method */
extern int from_digitizer;
extern int from_screen;
extern int from_flag;

extern int dotsize;

extern int THE_COLORS[10];

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
