
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
#include "flag.h"
#include "regtree.h"
#include "ngbrtree.h"

/* #def _OR_SHAPE_ */


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
    /* user parameters */
    char *image_group;
    int weighted;		/* 0 if false/not selected, so we should scale input.
				 * 1 if the scaling should be skipped */
    int method;			/* Segmentation method */
    int nn;			/* number of neighbors, 4 or 8 */
    double max_diff;		/* max possible difference */
    double alpha;		/* similarity threshold */
    int min_segment_size;	/* smallest number of pixels/cells allowed in a final segment */

    double radio_weight;	/* weighing factor radiometric - shape */
    double smooth_weight;       /* weighing factor smoothness - compactness */

    int end_t;			/* maximum number of iterations */
    int mb;

    /* region info */
    int nrows, ncols;
    int row_min, row_max, col_min, col_max; /* region constraints */
    int ncells;

    char *out_name;		/* name of output raster map */
    char *seeds, *bounds_map;	/* optional segment seeds and polygon constraints/boundaries */
    CELL lower_bound, upper_bound;
    const char *bounds_mapset;
    char *out_band;		/* indicator for segment heterogeneity */

    /* file processing */
    int nbands;			/* number of rasters in the image group */
    SEGMENT bands_seg, 	        /* input group with one or more bands */
            bounds_seg,
	    rid_seg;
    DCELL *bands_min, *bands_max;
    DCELL *bands_val;		/* array to hold all input values for one cell */
    DCELL *second_val;		/* array to hold all input values for another cell */

    /* results */
    struct RG_TREE *reg_tree;   /* search tree with region stats */
    int min_reg_size;		/* minimum region size */
    struct reg_stats rs, rs_i, rs_k;
    struct ngbr_stats ns;
    size_t datasize;		/* nbands * sizeof(double) */
    int n_regions;

    /* processing flags */
    FLAG *candidate_flag, *null_flag;	/*TODO, need some way to remember MASK/NULL values.  Was using -1, 0, 1 in int array.  Better to use 2 FLAG structures, better readibility? */

    /* number of remaining cells to check */
    int candidate_count;

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
int region_growing(struct globals *);
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

/* void calculate_reg_stats(int, int, struct reg_stats *, 
                         struct globals *); */


/* rclist.c */
void rclist_init(struct rclist *);
void rclist_add(struct rclist *, int, int);
int rclist_drop(struct rclist *, struct rc *);
void rclist_destroy(struct rclist *);


/* write_output.c */
int write_output(struct globals *);
int close_files(struct globals *);
