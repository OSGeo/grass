#include "orthophoto.h"
#include <stdio.h>
#include <curses.h>

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
	struct Colors colors;
	char name[30];
	char mapset[30];
	int top, bottom, left, right;
	double ew_res, ns_res;	/* original map resolution */
    } cell;
} View;


/* this is a group structure */
/* not used -- see orthophoto.h 
   typedef struct
   {
   char name[50];
   struct Block_Image_Group_Ref ref;
   struct Ref_Points ref_points;
   struct Con_Points con_points;
   int ref_equation_stat;
   int con_equation_stat;
   double E12[3], N12[3], E21[3], N21[3], Z12[3], Z21[3];
   } Group;
   ******************************************************** */

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
