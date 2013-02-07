
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
#include <grass/linkm.h>
#include "flag.h"

/* PROFILE will add some rough time checks for finding neighbors, merging, and pass times. */
/* #define PROFILE */

/* SIGNPOST will add some fprintf statments to indicate what segments are being merged.
 * other diagnostics could be included here for during further development and speed improvements.
 */
/* #define SIGNPOST */

/* pixel stack */
struct pixels
{
    struct pixels *next;
    int row;
    int col;
    int count_shared;		/* todo perimeter: will hold the count how 
				 * many pixels are shared on the Border Between 
				 * Ri and Rk.  Not used for all pixels... see if 
				 * this is an OK way to do this... */
};

/* input and output files, as well as some other processing info */
struct files
{
    /* user parameters */
    char *image_group;
    int weighted;		/* 0 if false/not selected, so we should scale input.  1 if the scaling should be skipped */

    /* region info */
    int nrows, ncols;

    /* files */
    char *out_name;		/* name of output raster map */
    const char *seeds_map, *seeds_mapset, *bounds_map, *bounds_mapset;	/* optional segment seeds and polygon constraints/boundaries */
    char *out_band;		/* for segment average values */

    /* file processing */
    /* bands_seg is initialized with the input raster valuess, then is updated with current mean values for the segment. */
    /* for now, also storing: Area, Perimeter, maxcol, mincol, maxrow, minrow */
    int nbands;			/* number of rasters in the image group */
    SEGMENT bands_seg, bounds_seg;	/* bands is for input, normal application is landsat bands, but other input can be included in the group. */
    double *bands_val;		/* array, to hold all input values at one pixel */
    double *second_val;		/* to hold values at second point for similarity comparison */
    int bounds_val, current_bound;
    int minrow, maxrow, mincol, maxcol;

    /* results */
    SEGMENT iseg_seg;		/* segment ID assignment */
    int nsegs;			/* number of segments */

    /* processing flags */
    /* candidate flag for if a cell/segment has already been merged in that pass. */
    /* seeds flag for if a cell/segment is a seed (can be Ri to start a merge).
     *   All cells are valid seeds if a starting seeds map is not supplied. */
    FLAG *candidate_flag, *null_flag, *orig_null_flag, *seeds_flag;

    /* memory management, linked lists */
    struct link_head *token;	/* for linkm.h linked list memory management. */

};

struct functions
{
    int method;			/* Segmentation method */
    int num_pn;			/* number of pixel neighbors  int, 4 or 8. */
    float threshold;		/* similarity threshold */
    int min_segment_size;	/* smallest number of pixels/cells allowed in a final segment */
    float radio_weight, smooth_weight;	/* radiometric (bands) vs. shape and smoothness vs. compactness */
    /* Some function pointers to set in parse_args() */
    int (*find_pixel_neighbors) (int, int, int[8][2], struct files *);	/*parameters: row, col, pixel_neighbors */
    double (*calculate_similarity) (struct pixels *, struct pixels *, struct files *, struct functions *);	/*parameters: two pixels (each with row,col) to compare */

    /* max number of iterations/passes */
    int end_t;

    int path;			/* flag if we are using Rk as next Ri for non-mutually best neighbor. */
    int limited;		/* flag if we are limiting merges to one per pass */
    int estimate_threshold;	/* flag if we just want to estimate a suggested threshold value and exit. */
    int final_merge_only;	/* flag if we want to just run the final merge portion of the algorithm. */

    /* todo: is there a fast way (and valid from an algorithm 
       standpoint) to merge all neighbors that are within some small % 
       of the treshold? * There is some code using "very_close" that is 
       excluded with IFDEF * The goal is to speed processing, since in 
       the end many of these very similar neighbors will be merged. * 
       But the problem is that the find_segment_neighbors() function 
       only returns a single pixel, not the entire segment membership. 
       * The commented out code actually slowed down processing times 
       in the first tries. */
#ifdef VCLOSE
    double very_close;		/* segments with very_close similarity 
				 * will be merged without changing or checking the candidate flag. 
				 * The algorithm will continue looking for the "most similar" 
				 * neighbor that isn't "very close". */
#endif
};

/* main.c */
int estimate_threshold(char *);
int check_group(char *);
int read_range(double *, double *, char *);
double calc_t(double, double);

/* parse_args.c */
/* gets input from user, validates, and sets up functions */
int parse_args(int, char *[], struct files *, struct functions *);

/* open_files.c */
int open_files(struct files *, struct functions *);

/* create_isegs.c */
int create_isegs(struct files *, struct functions *);
int region_growing(struct files *, struct functions *);
int find_segment_neighbors(struct pixels **, struct pixels **, int *,
			   struct files *, struct functions *);
int set_candidate_flag(struct pixels *, int, struct files *);
int merge_values(struct pixels *, struct pixels *, int, int, struct files *);
int merge_pixels(struct pixels *, int, struct files *);
int find_four_pixel_neighbors(int, int, int[][2], struct files *);
int find_eight_pixel_neighbors(int, int, int[8][2], struct files *);
double calculate_euclidean_similarity(struct pixels *, struct pixels *,
				      struct files *, struct functions *);
double calculate_manhattan_similarity(struct pixels *, struct pixels *,
				      struct files *, struct functions *);
int my_dispose_list(struct link_head *, struct pixels **);
int compare_ids(const void *, const void *);
int compare_pixels(const void *, const void *);
int set_all_candidate_flags(struct files *);

/* write_output.c */
int write_output(struct files *);
int close_files(struct files *);
