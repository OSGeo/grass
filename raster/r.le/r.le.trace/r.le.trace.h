/*
 ************************************************************
 * MODULE: r.le.patch/r.le.trace.h                          *
 *         Version 5.0                Nov. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To analyze attributes of patches in a landscape *
 *         r.le.trace.h lists include files, defines the    *
 *         data structures, and lists the modules for       *
 *         r.le.trace                                       *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#include <grass/config.h>
#include "stdio.h"
#include "math.h"
#include "ctype.h"
#include "stdlib.h"
#include "string.h"
#include <grass/gis.h>

#define EQ(a, b)    (a-b < 0.01 && a-b > -0.01 )
#define BIG   500000000.0
#define MIN   5
#define NULLPTR (PATCH *) 0

typedef struct pt
{
    int row, col;
    struct pt *next;
} PT;

typedef struct patch
{
    double att;
    int num, n, s, e, w, npts;
    double c_row, c_col;
    double area, perim, long_axis;
    double edge, core;
    int *row;
    int *col;
    int twist;
    float omega;
    struct patch *next;
} PATCH;

struct CHOICE
{
    char fn[30], out[30];
    int patchmap, trace, perim2;
    int all;
    int boundary[5];
};

void set_map();
void show_patch();
void patch_attr();
void draw_patch();
void scr_cell();
void cell_clip_drv();
void cell_clip();
int is_not_empty_buffer();
int center_is_not_zero();
void trace();
PATCH *get_bd();
int yes_nb();
void clockwise();
