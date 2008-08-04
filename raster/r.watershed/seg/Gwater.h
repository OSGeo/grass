#ifndef __G_WATER_H__
#define __G_WATER_H__


/* program to map out drainage basin structure  */
/* this one uses the A * search algorithm       */
/* written by Chuck Ehlschlaeger                */
/* last modified 03/26/91                       */
#include <math.h>
#include <grass/gis.h>
#include "cseg.h"

#define AR_SIZE			16
#define AR_INCR			16
#define SHORT			int
#define NOMASK			1
#define MIN_SLOPE		.00001
#define MIN_GRADIENT_DEGREES	1
#define DEG_TO_RAD		.017453293	/* pi / 180 */
#define METER_TO_FOOT		3.281
#define MAX_BYTES		2000000
#define PAGE_BLOCK		512
#define SROW			9
#define SCOL   			13
#define RITE			1
#define LEFT			2
#define NEITHER			0
#define ABS(x)	(((x) < 0) ? -(x) : (x))
#define TSTSTR(a)	(fprintf (stderr, "%s\n", a))
#define TST(a)		(fprintf (stderr, "%e\n", (double) (a)))

#define POINT       struct points
POINT {
    SHORT r, c, downr, downc;
    int nxt;
};

#ifdef MAIN
#define GLOBAL
#define DRAINVAR	= {{ 7,6,5 },{ 8,0,4 },{ 1,2,3 }}
#define UPDRAINVAR	= {{ 3,2,1 },{ 4,0,8 },{ 5,6,7 }}
#define NEXTDRVAR	= { 1,-1,0,0,-1,1,1,-1 }
#define NEXTDCVAR	= { 0,0,-1,1,1,-1,1,-1 }
#else
#define GLOBAL extern
#define DRAINVAR
#define UPDRAINVAR
#define NEXTDRVAR
#define NEXTDCVAR
#endif

GLOBAL struct Cell_head window;

GLOBAL int first_astar, first_cum, nxt_avail_pt, total_cells, do_points;
GLOBAL SHORT nrows, ncols;
GLOBAL double half_res, diag, max_length, dep_slope;
GLOBAL int bas_thres, tot_parts;
GLOBAL SSEG astar_pts;
GLOBAL BSEG worked, in_list, s_b, swale;
GLOBAL CSEG dis, alt, wat, asp, bas, haf, r_h, dep;
GLOBAL DSEG slp, s_l, s_g, l_s, ril;
GLOBAL CELL one, zero;
GLOBAL double ril_value, dzero;
GLOBAL SHORT sides;
GLOBAL SHORT drain[3][3] DRAINVAR;
GLOBAL SHORT updrain[3][3] UPDRAINVAR;
GLOBAL SHORT nextdr[8] NEXTDRVAR;
GLOBAL SHORT nextdc[8] NEXTDCVAR;
GLOBAL char ele_name[GNAME_MAX], *ele_mapset, pit_name[GNAME_MAX],
    *pit_mapset;
GLOBAL char run_name[GNAME_MAX], *run_mapset, ob_name[GNAME_MAX], *ob_mapset;
GLOBAL char ril_name[GNAME_MAX], *ril_mapset, dep_name[GNAME_MAX],
    *dep_mapset;

GLOBAL char *this_mapset;
GLOBAL char seg_name[GNAME_MAX], bas_name[GNAME_MAX], haf_name[GNAME_MAX],
    thr_name[8];
GLOBAL char ls_name[GNAME_MAX], st_name[GNAME_MAX], sl_name[GNAME_MAX],
    sg_name[GNAME_MAX];
GLOBAL char wat_name[GNAME_MAX], asp_name[GNAME_MAX], arm_name[GNAME_MAX],
    dis_name[GNAME_MAX];
GLOBAL char ele_flag, pit_flag, run_flag, dis_flag, ob_flag;
GLOBAL char wat_flag, asp_flag, arm_flag, ril_flag, dep_flag;
GLOBAL char bas_flag, seg_flag, haf_flag, er_flag;
GLOBAL char st_flag, sb_flag, sg_flag, sl_flag, ls_flag;
GLOBAL FILE *fp;

/* close_maps.c */
int close_maps(void);

/* close_maps2.c */
int close_array_seg(void);

/* def_basin.c */
CELL def_basin(int, int, CELL, double, CELL);

/* do_astar.c */
int do_astar(void);
int add_pt(SHORT, SHORT, SHORT, SHORT, CELL, CELL);
double get_slope(SHORT, SHORT, SHORT, SHORT, CELL, CELL);
int replace(SHORT, SHORT, SHORT, SHORT);

/* do_cum.c */
int do_cum(void);

/* find_pour.c */
int find_pourpts(void);

/* haf_side.c */
int haf_basin_side(SHORT, SHORT, SHORT);

/* init_vars.c */
int init_vars(int, char *[]);
int do_legal(char *);
char *do_exist(char *);

/* no_stream.c */
int no_stream(int, int, CELL, double, CELL);

/* over_cells.c */
int overland_cells(int, int, CELL, CELL, CELL *);

/* sg_factor.c */
int sg_factor(void);
int len_slp_equ(double, double, double, int, int);

/* slope_len.c */
int slope_length(SHORT, SHORT, SHORT, SHORT);

/* split_str.c */
CELL split_stream(int, int, int[], int[], int, CELL, double, CELL);

/* usage.c */
void usage(char *);


#endif /* __G_WATER_H__ */
