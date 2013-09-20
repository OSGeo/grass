#include <math.h>
#include <grass/raster.h>
#include "ramseg.h"
#include "flag.h"

#define NODE		struct _n_o_d_e_
#define AR_SIZE			16
#define AR_INCR		64
#define ABS(x)		(((x) < 0) ? -(x) : (x))
#ifdef MIN
 #undef MIN
 #define MIN(x,y)	(((x) < (y)) ? (x) : (y))
#endif
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

extern SHORT drain[3][3];
extern SHORT updrain[3][3];
extern char dr_mod[9];
extern char dc_mod[9];
extern char basin_name[GNAME_MAX], swale_name[GNAME_MAX],
    half_name[GNAME_MAX], elev_name[GNAME_MAX], armsed_name[GNAME_MAX];
extern int nrows, ncols, done, total;
extern int array_size, high_index, do_index;
extern char *drain_ptrs, ha_f, el_f, ar_f;
extern RAMSEG ba_seg, pt_seg, sl_seg;
extern int ncols_less_one, nrows_less_one;
extern NODE *to_do;
extern FILE *arm_fd, *fp;
extern FLAG *doner, *swale, *left;
extern CELL *bas;
extern double half_res, diag, max_length, dep_slope;
extern struct Cell_head window;

/*
   GLOBAL CELL     *dis, *alt, *wat, *asp, *bas, *haf, *r_h, *dep, *ril_buf;
 */
