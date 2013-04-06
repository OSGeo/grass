#ifndef __G_WATER_H__
#define __G_WATER_H__


/* program to map out drainage basin structure  */
/* this one uses the A * search algorithm       */
/* written by Chuck Ehlschlaeger                */
/* last modified 03/26/91                       */
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "ramseg.h"
#include "flag.h"

/* redefining G_malloc allows you to see where in */
/* program that memory runs out */
/* #define G_malloc malloc */

#define AR_SIZE			16
#define AR_INCR			16
#define NOMASK			1
#define MIN_SLOPE		.00001
#define MIN_GRADIENT_DEGREES	1
#define DEG_TO_RAD		((2 * M_PI) / 360.)
#define METER_TO_FOOT		(1 / 0.3048)
#define MAX_BYTES		2000000
#define PAGE_BLOCK		512
#define RITE			1
#define LEFT			2
#define NEITHER			0
#define ABS(x)	(((x) < 0) ? -(x) : (x))
#define TSTSTR(a)	(fprintf (stderr, "%s\n", a))
#define TST(a)		(fprintf (stderr, "%e\n", (double) (a)))

#define POINT       struct points
POINT {
    int r, c; /* , downr, downc */
    /* int nxt; */
};

#define OC_STACK struct overland_cells_stack
OC_STACK {
    int row, col;
};

extern struct Cell_head window;

extern int mfd, c_fac, abs_acc, ele_scale;
extern int *heap_index, heap_size;
extern int first_astar, first_cum, nxt_avail_pt, total_cells, do_points;
extern int nrows, ncols;
extern double half_res, diag, max_length, dep_slope;
extern int bas_thres, tot_parts;
extern CELL n_basins;
extern OC_STACK *ocs;
extern int ocs_alloced;
extern FLAG *worked, *in_list, *s_b, *swale, *flat_done;
extern RAMSEG dis_seg, alt_seg, wat_seg, asp_seg, bas_seg, haf_seg;
extern RAMSEG r_h_seg, dep_seg;
extern RAMSEG slp_seg, s_l_seg, s_g_seg, l_s_seg;
extern int *astar_pts;
extern CELL *dis, *alt, *asp, *bas, *haf, *r_h, *dep;
extern DCELL *wat, *tci;
extern int ril_fd;
extern double *s_l, *s_g, *l_s;
extern CELL one, zero;
extern double ril_value, d_one, d_zero;
extern int sides;
extern int drain[3][3];
extern int updrain[3][3];
extern int nextdr[8];
extern int nextdc[8];
extern char ele_name[GNAME_MAX], pit_name[GNAME_MAX];
extern char run_name[GNAME_MAX], ob_name[GNAME_MAX];
extern char ril_name[GNAME_MAX], dep_name[GNAME_MAX];
extern const char *this_mapset;
extern char seg_name[GNAME_MAX], bas_name[GNAME_MAX], haf_name[GNAME_MAX], thr_name[8];
extern char ls_name[GNAME_MAX], st_name[GNAME_MAX], sl_name[GNAME_MAX], sg_name[GNAME_MAX];
extern char wat_name[GNAME_MAX], asp_name[GNAME_MAX], tci_name[GNAME_MAX];
extern char arm_name[GNAME_MAX], dis_name[GNAME_MAX];
extern char ele_flag, pit_flag, run_flag, dis_flag, ob_flag, flat_flag;
extern char wat_flag, asp_flag, arm_flag, ril_flag, dep_flag, tci_flag;
extern char bas_flag, seg_flag, haf_flag, er_flag;
extern char st_flag, sb_flag, sg_flag, sl_flag, ls_flag;
extern FILE *fp;

/* close_maps.c */
int close_maps(void);

/* close_maps2.c */
int close_array_seg(void);

/* def_basin.c */
CELL def_basin(int, int, CELL, double, CELL);

/* do_astar.c */
int do_astar(void);
int add_pt(int, int, CELL);
int drop_pt(void);
double get_slope(int, int, int, int, CELL, CELL);

/* do_flatarea.c */
int do_flatarea(int, CELL, CELL *, CELL *);

/* do_cum.c */
int do_cum(void);
int do_cum_mfd(void);
double mfd_pow(double, int);

/* find_pour.c */
int find_pourpts(void);

/* haf_side.c */
int haf_basin_side(int, int, int);

/* init_vars.c */
int init_vars(int, char *[]);

/* no_stream.c */
int no_stream(int, int, CELL, double, CELL);

/* over_cells.c */
int overland_cells(int, int, CELL, CELL, CELL *);

/* ramseg.c */
int size_array(int *, int, int);
int seg_index_rc(int, int, int *, int *);

/* sg_factor.c */
int sg_factor(void);
int len_slp_equ(double, double, double, int, int);

/* slope_len.c */
int slope_length(int, int, int, int);

/* split_str.c */
CELL split_stream(int, int, int[], int[], int, CELL, double, CELL);

/* usage.c */
void usage(char *);


#endif /* __G_WATER_H__ */
