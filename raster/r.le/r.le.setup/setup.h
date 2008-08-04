/*
 ************************************************************
 * MODULE: r.le.setup/setup.h                               *
 *         Version 5.0beta            Oct. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To set up sampling areas, which can can then    *
 *         be used to obtain data using the r.le.dist,      *
 *         r.le.patch, and r.le.pixel programs.  The        *
 *         setup.h code contains structure definitions      *
 *         and lists of modules in r.le.setup               *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#include "stdio.h"
#include "math.h"
#include "signal.h"
#include "setjmp.h"
#include "ctype.h"
#include "stdlib.h"
#include "string.h"
#include <grass/gis.h>
#include "sys/types.h"


/* #include "dig_defines.h" */
/* #include "dig_structs.h" */
/* #include "dig_head.h" */



#define  SML   0.5
#define EQ(a, b)    (a-b < 0.01 && a-b > -0.01 )
#define BIG   1000000000.0

extern jmp_buf jmp;

/** sample.c **/
extern void sample(int t0, int b0, int l0, int r0, char *name, char *name1,
		   char *name2, double *msc);
extern void draw_box(int x0, int y0, int xp, int yp, int thick);
extern void draw_circle(int x0, int y0, int xp, int yp, int thick);
extern void numtrap(int n, double *a);

/** mv_wind.c **/
extern void mov_wind(int t, int b, int l, int r, char *n1, char *n2, char *n3,
		     double *mx);

/** ask_group.c **/
extern int ask_group(char **sel);
extern int get_group_drv(char **sel);
extern FILE *fopen0(char *name, char *flag);

/** setup.c **/

extern void set_map(char *name, char *name1, char *name2,
		    struct Cell_head window, int top, int bot, int left,
		    int right);
extern void paint_map(char *n1, char *n2, char *n3);
