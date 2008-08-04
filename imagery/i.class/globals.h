#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "defs.h"

#ifndef GLOBAL
#  define GLOBAL extern
#  define INIT(x)
#else
#  define INIT(x) = x
#endif

GLOBAL Window *PROMPT_WINDOW;

GLOBAL int SCREEN_TOP;
GLOBAL int SCREEN_BOTTOM;
GLOBAL int SCREEN_LEFT;
GLOBAL int SCREEN_RIGHT;

GLOBAL View *VIEW_MAP1;
GLOBAL View *VIEW_TITLE1;
GLOBAL View *VIEW_MAP1_ZOOM;
GLOBAL View *VIEW_TITLE1_ZOOM;

GLOBAL View *VIEW_MASK1;
GLOBAL View *VIEW_MENU;
GLOBAL View *VIEW_HISTO;

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
#define MY_COLORS {{0,0,0},         /*black*/ \
		     {50,50,255},     /*blue*/  \
		     {170,200,70}, /*brown*/ \
		     {0,255,0},     /*green*/ \
		     {150,150,150}, /*grey*/  \
		     {220,170,0},   /*orange*/\
		     {200,0,200},   /*purple*/\
		     {255,0,0},     /*red*/   \
		     {255,255,255}, /*white*/ \
		     {255,255,0}}	/*yellow */
GLOBAL struct
{
    int red, grn, blue;
} Color_table[10] INIT(MY_COLORS);

GLOBAL struct Ref Refer;
GLOBAL FILE *outsig_fd;
GLOBAL struct Signature Sigs;
GLOBAL struct Cell_head Band_cellhd;

double row_to_northing();
double col_to_easting();

GLOBAL int *Bandfd;
GLOBAL struct region Region;
GLOBAL struct signalflag signalflag;
GLOBAL CELL **Bandbuf;


#undef INIT

#endif /* __GLOBALS_H__ */
