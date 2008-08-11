#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "defs.h"

extern Window *PROMPT_WINDOW;

extern int SCREEN_TOP;
extern int SCREEN_BOTTOM;
extern int SCREEN_LEFT;
extern int SCREEN_RIGHT;

extern View *VIEW_MAP1;
extern View *VIEW_TITLE1;
extern View *VIEW_MAP1_ZOOM;
extern View *VIEW_TITLE1_ZOOM;

extern View *VIEW_MASK1;
extern View *VIEW_MENU;
extern View *VIEW_HISTO;

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

#define NUM_BLACK	0
#define NUM_BLUE	1
#define NUM_BROWN	2
#define NUM_GREEN	3
#define NUM_GREY	4
#define NUM_ORANGE	5
#define NUM_PURPLE	6
#define NUM_RED 	7
#define NUM_WHITE	8
#define NUM_YELLOW	9

#define NAME_BLACK	"Black"
#define NAME_BLUE	"Blue"
#define NAME_BROWN	"Brown"
#define NAME_GREEN	"Green"
#define NAME_GREY	"Grey"
#define NAME_ORANGE	"Orange"
#define NAME_PURPLE	"Purple"
#define NAME_RED 	"Red"
#define NAME_WHITE	"White"
#define NAME_YELLOW	"Yellow"

struct Color_table
{
    int red, grn, blue;
};

extern struct Color_table Color_table[10];

extern struct Ref Refer;
extern FILE *outsig_fd;
extern struct Signature Sigs;
extern struct Cell_head Band_cellhd;

extern int *Bandfd;
extern struct region Region;
extern struct signalflag signalflag;
extern CELL **Bandbuf;

extern double row_to_northing(struct Cell_head *, int, double);
extern double col_to_easting(struct Cell_head *, int, double);

#endif /* __GLOBALS_H__ */
