/*
 ************************************************************
 * MODULE: r.le.patch/patch.h                               *
 *         Version 5.0                Nov. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To analyze attributes of patches in a landscape *
 *         patch.h lists include files, defines the data    *
 *         structures, and lists the modules for r.le.patch *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#ifndef __PATCH_H__
#define __PATCH_H__


#include <grass/config.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <setjmp.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>

#define  SML   0.5
#define  MIN   2
#define  EQ(a, b)    (a-b < 0.01 && a-b > -0.01 )
#define  BIG   1000000000.0
#define  PI    M_PI

extern jmp_buf jmp;

typedef struct __dirdesc
{
    int dd_fd;			/* file descriptor */
    long dd_loc;		/* buf offset of entry from last readddir() */
    long dd_size;		/* amount of valid data in buffer */
    long dd_bsize;		/* amount of entries read at a time */
    long dd_off;		/* Current offset in dir (for telldir) */
    char *dd_buf;		/* directory data buffer */
} DIR;

extern DIR *opendir( /* char *dirname */ );
extern struct dirent *readdir( /* DIR *dirp */ );
extern int closedir( /* DIR *dirp */ );


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
    char fn[GNAME_MAX], reg[GMAPSET_MAX], out[GNAME_MAX], wrum;
    int core2, size2, shape2, edge, fb, coremap, units;
    int perim2, trace, patchmap;
    int Mx[4];
    int att[9], size[9], shape[8], boundary[5], perim[8], core[11];
};

typedef struct reglist
{
    int att;
    int n, s, e, w;
    struct reglist *next;
} REGLIST;


/** main.c **/
void user_input();

/** driver.c **/
void patch_fore();
void open_files();
void read_line();
void read_recl_tb();
void read_para();
void get_para();
void free_para();

void mv_driver();
void read_mwind();
void set_colors();
void get_para_mv();
int need_gp();

void whole_reg_driver();
void unit_driver();
void run_clip(int, int, int, int, int, int, CELL **, int, float);

void meter();
FILE *fopen0();
FILE *fopen1();
FILE *fopen2();


/** trace.c **/
void cell_clip_drv(int, int, int, int, double **, int, float);
void cell_clip(DCELL **, DCELL **, int, int, int, int, int, float, int *,
	       int *);
int is_not_empty_buffer();
int center_is_not_null();
void trace();
PATCH *get_bd();
void clockwise();
void counterclockwise();
int yes_nb();


/** patch.c **/
void df_patch();
void mv_patch();

double eu_dist();
double eu_d();
int in_group();
int check_order();
int index_coh();
int recl_coh();

void m_att();
void m_core();
void m_size();
void m_shape();
void m_perim();
void m_boundary();

void df_att();
void df_core();
void df_size();
void df_shape();
void df_perim();
void df_boundary();

void save_att();
void save_core();
void save_size();
void save_shape();
void fit();


#endif /* __PATCH_H__ */
