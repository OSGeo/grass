
/****************************************************************************
 *
 * MODULE:       i.segment
 * AUTHOR(S):    Eric Momsen <eric.momsen at gmail com>
 * PURPOSE:      structure definition and function listing
 * COPYRIGHT:    (C) 2012 by Eric Momsen, and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the COPYING file that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <grass/segment.h>
#include <grass/imagery.h>
#include "flag.h"
#include "regtree.h"
#include "ngbrtree.h"

/* #def _OR_SHAPE_ */

#ifdef HAVE_LONG_LONG_INT
#define LARGEINT long long
#define PRI_LONG "lld"
#elif defined HAVE_LARGEFILES
#define LARGEINT off_t
#define PRI_LONG PRI_OFF_T
#else
#define LARGEINT long
#define PRI_LONG "ld"
#endif

/* methods */
#define ORM_RG 1	/* region growing */
#define ORM_MS 2	/* mean shift */
#define ORM_WS 3	/* watershed */

/* row/col list */
struct rc
{
    struct rc *next;
    int row;
    int col;
};

struct rclist
{
    struct rc *tail, *head;
};


/* input and output files, as well as some other processing info */
struct globals
{
    /* input */
    char *image_group;
    struct Ref Ref;		/* group reference list */
    DCELL *min, *max;
    int weighted;		/* 0 if false/not selected, so we should scale input.
				 * 1 if the scaling should be skipped */
    int nbands;			/* number of rasters in the image group */
    size_t datasize;		/* nbands * sizeof(double) */
    int mb;			/* amount of memory to use in MB */

    char *seeds, *bounds_map;	/* optional segment seeds and polygon constraints/boundaries */
    CELL lower_bound, upper_bound;
    const char *bounds_mapset;

    /* output */
    /* region growing */
    char *out_name;		/* name of output raster map with regions */
    char *gof;			/* indicator for segment heterogeneity / goodness of fit */
    char *bsuf;			/* suffix to be appended to input bands */

    /* general segmentation */
    int method;			/* Segmentation method code */
    int (*method_fn)();		/* Segmentation method function */
    int nn;			/* number of neighbors, 4 or 8 */
    double max_diff;		/* max possible difference */
    double alpha;		/* similarity threshold */
    int end_t;			/* maximum number of iterations */

    /* region growing */
    int min_segment_size;	/* smallest number of pixels/cells allowed in a final segment */
    int *new_id;

    /* inactive options for region growing */
    double radio_weight;	/* weighing factor radiometric - shape */
    double smooth_weight;       /* weighing factor smoothness - compactness */

    /* mean shift */
    double hs, hr;		/* spectral and range bandwidth */
    int ms_adaptive;		/* use adaptive bandwidth */
    int ms_progressive;		/* use progressive bandwidth */

    /* region info */
    int nrows, ncols;
    int row_min, row_max, col_min, col_max; /* region constraints */
    LARGEINT ncells, notnullcells;

    /* file processing */
    SEGMENT bands_seg, 	        /* input group with one or more bands */
            bands_seg2,		/* copy of bands_seg for mean shift */
            bounds_seg,
	    rid_seg;
    SEGMENT *bands_in, *bands_out; /* pointers to input/output bands_seg, for mean shift */
    DCELL *bands_min, *bands_max;
    DCELL *bands_val;		/* array to hold all input values for one cell */
    DCELL *second_val;		/* array to hold all input values for another cell */

    /* maximum used region ID */
    CELL max_rid;

    /* region growing internal structure */
    struct RG_TREE *reg_tree;   /* search tree with region stats */
    LARGEINT min_reg_size;	/* minimum region size */
    struct reg_stats rs, rs_i, rs_k;
    struct ngbr_stats ns;

    /* processing flags */
    FLAG *candidate_flag, *null_flag;	/*TODO, need some way to remember MASK/NULL values.  Was using -1, 0, 1 in int array.  Better to use 2 FLAG structures, better readibility? */

    /* number of remaining cells to check */
    LARGEINT candidate_count;

    /* functions */
    void (*find_neighbors) (int, int, int[8][2]);	/*parameters: row, col, neighbors */
    double (*calculate_similarity) (struct ngbr_stats *,
                                    struct ngbr_stats *,
				    struct globals *);	/*parameters: two regions to compare */
};


/* parse_args.c */
/* gets input from user, validates, and sets up functions */
int parse_args(int, char *[], struct globals *);

/* open_files.c */
int open_files(struct globals *);

/* create_isegs.c */
int create_isegs(struct globals *);
void find_four_neighbors(int, int, int[][2]);
void find_eight_neighbors(int, int, int[8][2]);
double calculate_euclidean_similarity(struct ngbr_stats *, 
                                      struct ngbr_stats *,
				      struct globals *);
double calculate_manhattan_similarity(struct ngbr_stats *, 
                                      struct ngbr_stats *,
				      struct globals *);
double calculate_shape(struct reg_stats *, struct reg_stats *,
                       int, struct globals *);
int fetch_reg_stats(int , int , struct reg_stats *, 
                           struct globals *);
int update_band_vals(int, int, struct reg_stats *, struct globals *);

/* void calculate_reg_stats(int, int, struct reg_stats *, 
                         struct globals *); */

/* region_growing.c */
int region_growing(struct globals *);

/* mean_shift.c */
int mean_shift(struct globals *);

/* cluster.c */
CELL cluster_bands(struct globals *globals);

/* watershed.c */
int watershed(struct globals *);

/* rclist.c */
void rclist_init(struct rclist *);
void rclist_add(struct rclist *, int, int);
int rclist_drop(struct rclist *, struct rc *);
void rclist_destroy(struct rclist *);

/* write_output.c */
int write_ids(struct globals *);
int write_gof_rg(struct globals *);
int write_bands_ms(struct globals *);
int close_files(struct globals *);
