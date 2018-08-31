#ifndef __G_WATER_H__
#define __G_WATER_H__


/* program to map out drainage basin structure  */
/* this one uses the A * search algorithm       */
/* written by Chuck Ehlschlaeger                */
/* updated by Markus Metz                       */
/* last modified 22/10/09                       */
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>

#define GW_LARGE_INT off_t

#include "cseg.h"
#include "flag.h"

#define AR_SIZE			16
#define AR_INCR			16
#define NOMASK			1
#define MIN_SLOPE		.00001
#define MIN_GRADIENT_DEGREES	1
#define DEG_TO_RAD		((2 * M_PI) / 360.)
#define METER_TO_FOOT		(1 / 0.3048)
#define MAX_BYTES		10485760
#define PAGE_BLOCK		1024
#define SROW			64
#define SCOL   			64
#define RITE			1
#define LEFT			2
#define NEITHER			0
#define ABS(x)	(((x) < 0) ? -(x) : (x))
#define TSTSTR(a)	(fprintf (stderr, "%s\n", a))
#define TST(a)		(fprintf (stderr, "%e\n", (double) (a)))

/* flag positions */
#define NULLFLAG         0      /* elevation is NULL */
#define EDGEFLAG         1      /* edge cell */
#define INLISTFLAG       2      /* in open A* list */
#define WORKEDFLAG       3      /* in closed A* list/ accumulation done */
#define SWALEFLAG        4      /* swale */
#define PITFLAG          5      /* user-defined real depression */
#define RUSLEBLOCKFLAG   6      /* is RUSLE block */
/* #define XXXFLAG   7 */ /* last bit unused */


#define POINT       struct points
POINT {
    int r, c;
};

#define HEAP_PNT    struct heap_point
HEAP_PNT {
   GW_LARGE_INT added;
   CELL ele;
   POINT pnt;
};

#define WAT_ALT    struct wat_altitude
WAT_ALT {
   CELL ele;
   DCELL wat;
};

#define A_TANB    struct sca_tanb
A_TANB {
   DCELL sca;
   DCELL tanb;
};

#define ASP_FLAG    struct aspect_flag
ASP_FLAG {
   char asp;
   char flag;
};

#define OC_STACK struct overland_cells_stack
OC_STACK {
    int row, col;
};

extern struct Cell_head window;

extern int mfd, c_fac, abs_acc, ele_scale;
extern SSEG search_heap;
extern int nrows, ncols;
extern GW_LARGE_INT heap_size;
extern GW_LARGE_INT first_astar, first_cum, nxt_avail_pt, total_cells, do_points;
extern CELL n_basins;
extern OC_STACK *ocs;
extern int ocs_alloced;
extern double half_res, diag, max_length, dep_slope;
extern int bas_thres, tot_parts;
extern SSEG astar_pts;
extern BSEG s_b, rtn;
extern CSEG dis, bas, haf, r_h, dep;
extern SSEG watalt, aspflag;
extern DSEG slp, s_l, s_g, l_s, ril;
extern SSEG atanb;
extern double segs_mb;
extern char zero, one;
extern double ril_value, d_zero, d_one;
extern int sides;
extern char drain[3][3];
extern char updrain[3][3];
extern int nextdr[8];
extern int nextdc[8];
extern char ele_name[GNAME_MAX], pit_name[GNAME_MAX];
extern char run_name[GNAME_MAX], ob_name[GNAME_MAX];
extern char ril_name[GNAME_MAX], rtn_name[GNAME_MAX], dep_name[GNAME_MAX];

extern const char *this_mapset;
extern char seg_name[GNAME_MAX], bas_name[GNAME_MAX], haf_name[GNAME_MAX], thr_name[8];
extern char ls_name[GNAME_MAX], st_name[GNAME_MAX], sl_name[GNAME_MAX], sg_name[GNAME_MAX];
extern char wat_name[GNAME_MAX], asp_name[GNAME_MAX];
extern char tci_name[GNAME_MAX], spi_name[GNAME_MAX];
extern char arm_name[GNAME_MAX], dis_name[GNAME_MAX];
extern char ele_flag, pit_flag, run_flag, dis_flag, ob_flag, rtn_flag;
extern char wat_flag, asp_flag, arm_flag, ril_flag, dep_flag, tci_flag, spi_flag, atanb_flag;
extern char bas_flag, seg_flag, haf_flag, er_flag;
extern char st_flag, sb_flag, sg_flag, sl_flag, ls_flag;
extern FILE *fp;

/* the flags:
 * ele_flag    elevation map given
 * pit_flag    pit (depression) map given
 * run_flag    initial surface runoff given
 * dis_flag    ???
 * ob_flag     blocking map for RUSLE given
 * wat_flag    write accumulation output
 * asp_flag    write direction output
 * arm_flag    unused, for interactive mode
 * ril_flag    percentage disturbed land given
 * dep_flag    ???
 * st_flag     do stream extraction
 * bas_flag    write basin output
 * seg_flag    write stream output
 * haf_flag    write half-basin output
 * er_flag     do RUSLE
 * sb_flag     ???
 * sg_flag     write RUSLE S factor     
 * sl_flag     slope length, unused
 * ls_flag     write RUSLE LS factor
 */

/* close_maps.c */
int close_maps(void);

/* close_maps2.c */
int close_array_seg(void);

/* def_basin.c */
CELL def_basin(int, int, CELL, double, CELL);

/* do_astar.c */
int do_astar(void);
int add_pt(int, int, CELL);
double get_slope(int, int, int, int, CELL, CELL);

/* do_cum.c */
int do_cum(void);
int do_cum_mfd(void);
double mfd_pow(double, int);

/* do_stream.c */
int do_stream(void);

/* find_pour.c */
int find_pourpts(void);

/* haf_side.c */
int haf_basin_side(int, int, int);

/* init_vars.c */
int init_vars(int, char *[]);
char *do_exist(char *);

/* no_stream.c */
int no_stream(int, int, CELL, double, CELL);

/* over_cells.c */
int overland_cells(int, int, CELL, CELL, CELL *);

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
