/*
 ************************************************************
 * MODULE: r.le.pixel/pixel.h                               *
 *         Version 5.0                Nov. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To analyze pixel-scale landscape properties     *
 *         pixel.h lists include files, defines data        *
 *         structures, and lists modules                    *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#include <grass/config.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>

#define  BIG   1000000000.0
#define  MAX   800

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

struct CHOICE
{
    char fn[30], reg[30], wrum;
    int edge, tex, fb, units, z, edgemap;
    int att[5], div[5], te2[6];
    int jux[3], edg[3];
};

typedef struct reglist
{
    int att;
    int n, s, e, w;
    struct reglist *next;
} REGLIST;


/** main.c **/
void parse_cmd();
void parse_mv();
int get_int();

/** driver.c **/
void texture_fore();
void mv_driver();
void set_colors();
void read_mwind();
void meter2();
void unit_driver();
void run_clip(int, int, int, int, int, int, CELL **, int, int, float);
void whole_reg_driver();
FILE *fopen0();
FILE *fopen1();
FILE *fopen2();
FILE *fopen3();
void get_rich_whole();


/** cellclip.c **/
void cell_clip_drv(int, int, int, int, double **, int, int, float);
void cell_clip(DCELL **, DCELL **, int, int, int, int, int, float);
void get_rich();
int is_not_empty_buffer();
int center_is_not_zero();
int compar();


/** texture.c **/
void mv_texture();
void df_texture();
void cal_att();
void cal_divers();
void cal_tex();
void cal_edge();
void read_weight();
void read_edge();
int find_loc();
int find_edge();
int check_order();

/** input.c **/
void user_input();
