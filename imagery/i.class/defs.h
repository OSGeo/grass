#ifndef __DEFS_H__
#define __DEFS_H__

#include <grass/gis.h>
#include <curses.h>
#include <grass/imagery.h>
#include "point.h"

/* this is a curses structure */
typedef struct
{
    int top, left, bottom, right;
} Window;

/* this is a graphics structure */
typedef struct
{
    int top, bottom, left, right;
    int nrows, ncols;
    struct
    {
	int configured;
	struct Cell_head head;
	char name[GNAME_MAX];
	char mapset[GMAPSET_MAX];
	int top, bottom, left, right;
	double ew_res, ns_res;	/* original map resolution */
    } cell;
} View;


typedef struct
{
    int type;			/* object type */
    int (*handler) ();		/* routine to handle the event */
    char *label;		/* label to display if MENU or OPTION */
    int binding;		/* OPTION bindings */
    int *status;		/* MENU,OPTION status */
    int top, bottom, left, right;
} Objects;

#define MENU_OBJECT 1
#define OPTION_OBJECT 2
#define INFO_OBJECT 3
#define OTHER_OBJECT 4


#define MENU(label,handler,status) \
	{MENU_OBJECT,handler,label,0,status,0,0,0,0}
#define OPTION(label,binding,status) \
	{OPTION_OBJECT,NULL,label,binding,status,0,0,0,0}
#define INFO(label,status) \
	{INFO_OBJECT,NULL,label,0,status,0,0,0,0}
#define OTHER(handler,status) \
	{OTHER_OBJECT,handler,NULL,0,status,0,0,0,0}


struct signalflag
{
    int interrupt;
    int alarm;
};

#define OVER_WRITE 1
#define OVER_LAY 0

#define MAX_VERTEX 100
struct region
{
    struct
    {
	int define;
	int completed;
	int filled;
	int saved;
    } area;

    int npoints;
    POINT point[MAX_VERTEX];
    View *view;

    int saved_npoints;
    POINT saved_point[MAX_VERTEX];
    View *saved_view;

    int vertex_npoints;
    POINT vertex[MAX_VERTEX];

    int perimeter_npoints;
    POINT *perimeter;

};
extern int edge_order(const void *, const void *);

/* the mouse buttons and the numbers they return */
#define LEFT_BUTTON 1
#define MIDDLE_BUTTON 2
#define RIGHT_BUTTON 3

#define MAX_CATS 256

#define NORMAL_FONT "romans"
#define GREEK_FONT "greeks"
#define NORMAL_TEXT_SIZE 15

#define AFTER_STD 1
#define BEFORE_STD 0

#define MASK "MASK"

#endif /* __DEFS_H__ */
