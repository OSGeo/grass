#include <math.h>
#include <grass/gis.h>
#include "ramseg.h"
#include "flag.h"

#define NODE		struct _n_o_d_e_
#define AR_SIZE			16
#define AR_INCR		64
#define ABS(x)		(((x) < 0) ? -(x) : (x))
#define MIN(x,y)	(((x) < (y)) ? (x) : (y))
#define MAX_RAM		1000000
#define SROW		11
#define SCOL		10
#define SHORT			int
#define NOMASK			1
#define MIN_SLOPE		.00001
#define MIN_GRADIENT_DEGREES	1
#define DEG_TO_RAD		.017453293	/* pi / 180 */
#define METER_TO_FOOT		3.281
#define PAGE_BLOCK		512
#define RITE			1
#define LEFT			2
#define NEITHER			0

NODE {
    int row, col;
};

#ifdef MAIN
#define GLOBAL
#define DRAINVAR	= {{ 7,6,5 },{ 8,-17,4 },{ 1,2,3 }}
#define UPDRAINVAR	= {{ 3,2,1 },{ 4,-17,8 },{ 5,6,7 }}
#define DRVAR	={0,1,1,1,0,-1,-1,-1,0}
#define DCVAR	={0,1,0,-1,-1,-1,0,1,1}
#else
#define GLOBAL extern
#define DRAINVAR
#define UPDRAINVAR
#define DRVAR
#define DCVAR
#endif

GLOBAL SHORT drain[3][3] DRAINVAR;
GLOBAL SHORT updrain[3][3] UPDRAINVAR;
GLOBAL char dr_mod[9] DRVAR;
GLOBAL char dc_mod[9] DCVAR;
GLOBAL char basin_name[GNAME_MAX], swale_name[GNAME_MAX],
    half_name[GNAME_MAX], elev_name[GNAME_MAX], armsed_name[GNAME_MAX];
GLOBAL int nrows, ncols, done, total;
GLOBAL int array_size, high_index, do_index;
GLOBAL char *drain_ptrs, ha_f, el_f, ar_f;
GLOBAL RAMSEG ba_seg, pt_seg, sl_seg;
GLOBAL int ncols_less_one, nrows_less_one;
GLOBAL NODE *to_do;
GLOBAL FILE *arm_fd, *fp;
GLOBAL FLAG *doner, *swale, *left;
GLOBAL CELL *bas;
GLOBAL double half_res, diag, max_length, dep_slope;
GLOBAL struct Cell_head window;

/*
   GLOBAL CELL     *dis, *alt, *wat, *asp, *bas, *haf, *r_h, *dep, *ril_buf;
 */
