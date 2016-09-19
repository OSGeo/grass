/* Modified by: Janne Soimasuo August 1994; line_cat added
 * Modified by: Radim Blazek Jan 2000; acolor, label added
 * Modified by: Morten Hulden Mar 2004; cols added to vector
 * Modified by: Hamish Bowman Sept 2005; sizecol added to LAYER
 */

#include <grass/gis.h>
#include "clr.h"

#define PI M_PI

/* #define MAXVECTORS 20 */

/* layer type */
#define VPOINTS 0
#define VLINES 1
#define VAREAS 2

/* line justification */
#define LINE_REF_CENTER 0
#define LINE_REF_LEFT 1
#define LINE_REF_RIGHT 2

/* draw line */
#define LINE_DRAW_LINE 1
#define LINE_DRAW_HIGHLITE 2

/* construct_path() */
#define START_PATH  0
#define ADD_TO_PATH 1
#define CLOSE_PATH  2
#define WHOLE_PATH  3

/* line end style */
#define LINECAP_BUTT 0
#define LINECAP_ROUND 1
#define LINECAP_EXTBUTT 2


typedef struct
{
    /* All types */
    int type;			/* layer type: VPOINTS, VLINES, VAREAS */
    char *name;
    char *mapset;
    char masked;
    char *label;		/* label in legend */
    int lpos;			/* position in legend: -1 not specified, 0 do not display, > 0 position in legend */

    double width;		/* width of line, boundary or icon outline */
    PSCOLOR color;		/* color of line, boundary or icon outline */

    int field;			/* category field */
    char *cats;			/* list of categories */
    /* struct cat_list *clist; *//* list of categories */
    char *where;		/* SQL where condition (without WHERE key word) */

    /* Lines */
    double cwidth;		/* category width */
    double offset;		/* offset */
    double coffset;		/* category offset */
    int ref;			/* justification */
    char *linestyle;		/* line or boundary style */
    char *setdash;		/* line style converted to PS setdash format */
    int linecap;		/* line end style */
    double hwidth;		/* line or boundary highlight line width */
    PSCOLOR hcolor;

    /* Areas */
    char *pat;			/* name of eps file for pattern */
    double scale;		/* scale of pattern */
    double pwidth;		/* pattern width */

    /* Points */
    double size;		/* icon size */
    char *sizecol;		/* Column used for symbol size */
    char *rgbcol;		/* column used for symbol rgb color */
    /* already defined in Areas section above, so don't need it twice */
    /*    double scale; *//* Scale factor for dynamic sizing */
    double rotate;		/* symbol rotation */
    char *rotcol;		/* column used for symbol rotation */
    char *symbol;		/* symbol name */
    char *symbol_ps;		/* symbol name in PS */
    char *epspre;		/* first part of EPS file name */
    char *epssuf;		/* second part of EPS file name */
    int epstype;		/* 0 - no eps, 1 - common eps, 2 - eps for each category */

    /* Points + Line */
    int ltype;			/* point/centroid or line/boundary */

    /* Points + Areas */
    PSCOLOR fcolor;		/* fill color */

} LAYER;

struct vector
{
    int cur;			/* currently processed vector */
    int count;			/* number of recorded layers */
    int alloc;			/* allocated space */
    double x, y;		/* legend position */
    int fontsize;		/* legend font size */
    char *font;			/* legend font */
    double width;		/* width of legend symbols */
    int cols;			/* number of columns  */
    PSCOLOR border;		/* border color */
    double span;		/* column separation in inches */
    LAYER *layer;
};

extern struct vector vector;
