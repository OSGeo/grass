#include <tcl.h>
#include <tk.h>
#include <grass/Vect.h>

/*      tool in c:         name in Tk */
typedef enum
{
    TOOL_NOTHING,
    TOOL_EXIT,
    TOOL_NEW_POINT,
    TOOL_NEW_LINE,
    TOOL_NEW_BOUNDARY,
    TOOL_NEW_CENTROID,
    TOOL_MOVE_VERTEX,
    TOOL_ADD_VERTEX,		/* add vertex on line */
    TOOL_RM_VERTEX,		/* remove vertex from line */
    TOOL_SPLIT_LINE,
    TOOL_EDIT_LINE,		/* continue editing a line or boundary */
    TOOL_MOVE_LINE,
    TOOL_DELETE_LINE,
    TOOL_DISPLAY_CATS,
    TOOL_COPY_CATS,
    TOOL_DISPLAY_ATTRIBUTES,
    TOOL_ZOOM_WINDOW,		/* zoom by window */
    TOOL_ZOOM_OUT_CENTRE,
    TOOL_ZOOM_PAN,
    TOOL_ZOOM_DEFAULT,
    TOOL_ZOOM_REGION,
    TOOL_REDRAW,
    TOOL_DISPLAY_SETTINGS
} ToolNumber;

/* Xdriver coordinate value considered to bu null */
#define COOR_NULL PORT_INT_MAX

/* SYMBOLOGY */
typedef enum
{
    SYMB_DEFAULT,		/* line color acording to real line/node type from Line/NodeSymb */
    SYMB_BACKGROUND,
    SYMB_HIGHLIGHT,
    SYMB_POINT,
    SYMB_LINE,
    SYMB_BOUNDARY_0,		/* No areas */
    SYMB_BOUNDARY_1,		/* 1 area */
    SYMB_BOUNDARY_2,		/* 2 areas */
    SYMB_CENTROID_IN,		/* Centroid in area */
    SYMB_CENTROID_OUT,		/* Centroid outside area */
    SYMB_CENTROID_DUPL,		/* Duplicate centroid in area */
    SYMB_NODE_0,		/* Node without lines (points or centroids) */
    SYMB_NODE_1,		/* Node with 1 line */
    SYMB_NODE_2,		/* Node with 2 lines */
    SYMB_COUNT			/* MUST BE LAST, number of symbology layers */
} SymbNumber;

typedef struct
{
    int on;			/* 1 - on, 0 - off */
    int r, g, b;		/* color 0-255 */
} SYMB;

/* Message type */
#define MSG_OK 0
#define MSG_YESNO 1

/* Message icon */
#define MSGI_ERROR 0
#define MSGI_QUESTION 1

/* Snapping modes */
#define SNAP_SCREEN 0		/* Snap in screen pixels */
#define SNAP_MAP    1		/* Snap in map units */

/* Variables */
#define VART_INT    0
#define VART_DOUBLE 1
#define VART_CHAR   2

typedef struct
{
    int code;
    char *name;
    int type;
    int i;
    double d;
    char *c;
} VAR;

#define VAR_CAT       0		/* category for next line */
#define VARN_CAT      "cat"
#define VAR_FIELD     1
#define VARN_FIELD    "field"	/* field for next line */
#define VAR_CAT_MODE  2
#define VARN_CAT_MODE "cat_mode"	/* mode of cat imput */
#define VAR_INSERT    3
#define VARN_INSERT   "insert"	/* insert new row to table for next line (1) or not (0) */
#define VAR_MESSAGE   4
#define VARN_MESSAGE  "message"	/* text of message to be displayed in popup window */
#define VAR_SNAP      5
#define VARN_SNAP     "snap"	/* If to snap to nearest node (1) or not (0) */
#define VAR_SNAP_MODE    6
#define VARN_SNAP_MODE   "snap_mode"	/* Snapping mode (screen pixels / map units) */
#define VAR_SNAP_SCREEN  7
#define VARN_SNAP_SCREEN "snap_screen"	/* Snapping threshold in screen pixels */
#define VAR_SNAP_MAP     8
#define VARN_SNAP_MAP    "snap_map"	/* Snapping threshold in map units */
#define VAR_ZOOM_REGION  9
#define VARN_ZOOM_REGION "zoom_region"	/* Name of region to zoom in */
#define VAR_ANSWER      10
#define VARN_ANSWER      "answer"	/* Answer from dialog */
#define VAR_LINEWIDTH   11
#define VARN_LINEWIDTH   "linewidth"	/* Width for lines and boundaries */

extern VAR Variable[];

/* Category and field set for current line to be written */
#define CAT_MODE_NO   0		/* No category */
#define CAT_MODE_MAN  1		/* Manual imput */
#define CAT_MODE_NEXT 2		/* Next not yet used category of given field */
#define CAT_MODE_COUNT 3	/* Count of modes */

extern char *CatModeLab[];

/* Maximum value for field */
extern int (*MaxFieldCat)[2];
extern int nMaxFieldCat, aMaxFieldCat;

extern SYMB Symb[SYMB_COUNT];

extern struct Map_info Map;
extern struct Cell_head GRegion;	/* Current region (synchronized with GRASS WIND) */
extern Tcl_Interp *Toolbox;
extern int Tool_next;		/* Next tool to be run */
extern double Xscale, Yscale;	/* Scale factors = size_in_map / size_on_screen */

extern struct Cell_head window;

extern double Scale;		/* Map / xdriver */


/* Display symbology for lines and nodes */
extern int *LineSymb;		/* array of line symbology codes, starts from index 1 */
extern int aLineSymb;		/* number of lines / allocated space (array size + 1) */
extern int *NodeSymb;		/* array of nodes' symbology codes, start from index 1 */
extern int aNodeSymb;		/* number of nodes / allocated space (array size + 1) */

/* Background commands */
typedef struct
{
    char *cmd;			/* command */
    int on;			/* 1 display, 0 do not display */
} BGCMD;

extern BGCMD *Bgcmd;
extern int nbgcmd;
extern int abgcmd;

#define MOUSE_POINT	1
#define MOUSE_LINE	2
#define MOUSE_BOX	3
