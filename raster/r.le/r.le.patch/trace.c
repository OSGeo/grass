/*
 ************************************************************
 * MODULE: r.le.patch/trace.c                               *
 *         Version 5.0                Nov. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To analyze attributes of patches in a landscape *
 *         trace.c traces the patch boundary, obtains basic *
 *         patch attribute data, and saves this in the      *
 *         patch structure                                  *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#include <grass/gis.h>
#include <grass/config.h>
#include "patch.h"



extern struct CHOICE *choice;
extern struct REGLIST *reglist;
extern int finput;
int total_patches = 0;
PATCH *patch_list = NULL;


				/* DRIVER FOR CELL CLIPPING, TRACING,
				   AND CALCULATIONS */

void cell_clip_drv(int col0, int row0, int ncols, int nrows, double **value,
		   int index, float radius)
{
    CELL **pat, *pat_buf, *cor_cell_buf;
    FCELL *cor_fcell_buf;
    DCELL **buf, **cor, *cor_dcell_buf;
    DCELL **null_buf;
    int i, j, fd, fe, p, infd, centernull = 0, empty = 1;
    int hist_ok, colr_ok, cats_ok, range_ok;
    char *mapset, *name;
    PATCH *list_head;
    struct History hist;
    struct Categories cats;
    struct Categories newcats;
    struct Cell_stats stats;
    struct Colors colr;
    struct Range range;
    struct FPRange fprange;
    struct Quant quant;
    RASTER_MAP_TYPE data_type;

    /*
       Variables:
       EXTERN:
       finput =        the input raster map
       patch_list =    the patch list
       IN:
       col0 =          starting column for area to be clipped
       row0 =          starting row for area to be clipped
       ncols =         number of columns in area to be clipped
       nrows =         number of rows in area to be clipped
       value =         array containing a full row of the results of the moving
       window for all the chosen measures if a moving window map,
       otherwise 0
       index =         number of the region to be clipped, if there's a region map,
       number of the column if a moving window
       INTERNAL:
       pat =           pointer to array containing the map of patch numbers; this map
       can only be integer
       pat_buf =       row buffer to temporarily hold results for patch number map
       buf =           pointer to array containing the clipped area, a smaller area
       than the original raster map to be read from finput; this map
       can be integer, floating point, or double, but is stored as a
       double throughout the program
       cor =           pointer to array containing the map of interior area; this map
       can be integer, floating point, or double, but is stored as a
       double throughout the program
       cor_cell_buf =  pointer to row buffer to temporarily hold results for core map if
       input map is integer (CELL_TYPE)
       cor_fcell_buf = pointer to row buffer to temporarily hold results for core map if
       input map is float (FCELL_TYPE)
       cor_dcell_buf = pointer to row buffer to temporarily hold results for core map if
       input map is double (DCELL_TYPE)
       null_buf =      pointer to array containing 0.0 if pixel in input raster map is
       not null and 1.0 if pixel in input raster map is null
       infd =          pointer to file with map of patches
       i, j =          counts the row and column when going through arrays
       p =             counts the number of measures in
       fd =            pointer to file to hold patch number map
       fe =            pointer to file to hold patch interior map
       mapset =        the name of the mapset for the raster map being analyzed
       name =          the name of the raster map being analyzed
       list_head =     point to the patch list
     */


    total_patches = 0;

    name = choice->fn;
    mapset = G_mapset();
    data_type = G_raster_map_type(name, mapset);

    /* dynamically allocate storage for the
       buffer that will hold the contents of
       the clipped area */

    buf = (DCELL **) G_calloc(nrows + 3, sizeof(DCELL *));
    for (i = 0; i < nrows + 3; i++) {
	buf[i] = (DCELL *) G_allocate_raster_buf(DCELL_TYPE);
    }


    /* dynamically allocate storage for the
       buffer that will hold the null values for
       the clipped area */

    null_buf = (DCELL **) G_calloc(nrows + 3, sizeof(DCELL *));
    for (i = 0; i < nrows + 3; i++)
	null_buf[i] = (DCELL *) G_allocate_raster_buf(DCELL_TYPE);


    /* if a map of patch cores was requested,
       dynamically allocate storage for the
       buffer that will temporarily hold the map,
       then initialize the buffer with null values */

    if (choice->coremap) {
	cor = (DCELL **) G_calloc(nrows + 3, sizeof(DCELL *));
	for (i = 0; i < nrows + 3; i++) {
	    cor[i] = (DCELL *) G_allocate_raster_buf(DCELL_TYPE);
	}
	for (i = 0; i < nrows + 3; i++) {
	    G_set_null_value(cor[i], ncols + 3, DCELL_TYPE);
	}
    }

    /* if a map of patch numbers was requested,
       dynamically allocate storage for the
       buffer that will temporarily hold the map */

    if (choice->patchmap) {
	pat = (CELL **) G_calloc(nrows + 3, sizeof(CELL *));
	for (i = 0; i < nrows + 3; i++)
	    pat[i] = (CELL *) G_allocate_raster_buf(CELL_TYPE);
    }

    /* clip out the sampling area */

    cell_clip(buf, null_buf, row0, col0, nrows, ncols, index, radius,
	      &centernull, &empty);

    /* if the clipped area is not all null values
       trace the patches in the sampling area; if
       a moving window is used, then the center 
       pixel must also not be null */

    if (!empty) {
	if (choice->wrum != 'm') {
	    trace(nrows, ncols, buf, null_buf, pat, cor);
	}
	else {
	    if (!centernull)
		trace(nrows, ncols, buf, null_buf, pat, cor);
	}
    }
    /* if a map of patch cores was requested */

    if (choice->coremap) {
	/* 1. open the source map, then read the
	   supporting files (cell header, colors, 
	   categories, history, quant) into the
	   corresponding data structures */

	infd = G_open_cell_old(name, mapset);
	colr_ok = G_read_colors(name, mapset, &colr) > 0;
	cats_ok = G_read_raster_cats(name, mapset, &cats) >= 0;
	hist_ok = G_read_history(name, mapset, &hist) >= 0;
	range_ok = G_read_range(name, mapset, &range) >= 0;

	/* 2. if map is floating point, then read
	   the rules for quantization into the quant
	   data structure; if rules do not exist, create
	   them by rounding floating point values */

	if (data_type != CELL_TYPE) {
	    G_quant_init(&quant);
	    if (G_read_quant(name, mapset, &quant) <= 0)
		G_quant_round(&quant);
	}

	/* 3. initialize appropriate data structures */

	if (cats_ok)
	    G_init_raster_cats(G_get_raster_cats_title(&cats), &newcats);
	if (data_type == CELL_TYPE)
	    G_init_cell_stats(&stats);


	/* 4. open an output map called "interior" and
	   copy the contents of the core buffer into
	   this output map */

	switch (data_type) {
	case CELL_TYPE:
	    cor_cell_buf = G_allocate_raster_buf(CELL_TYPE);
	    fe = G_open_raster_new("interior", CELL_TYPE);
	    for (i = 1; i < nrows + 1; i++) {
		G_zero_raster_buf(cor_cell_buf, CELL_TYPE);
		for (j = 1; j < ncols + 1; j++)
		    *(cor_cell_buf + j - 1) = (int)(*(*(cor + i) + j));

		if (G_put_raster_row(fe, cor_cell_buf, CELL_TYPE) < 0)
		    exit(EXIT_FAILURE);

		G_update_cell_stats(cor_cell_buf, ncols + 1, &stats);
	    }
	    break;
	case FCELL_TYPE:
	    cor_fcell_buf = G_allocate_raster_buf(FCELL_TYPE);
	    fe = G_open_raster_new("interior", FCELL_TYPE);
	    for (i = 1; i < nrows + 1; i++) {
		G_zero_raster_buf(cor_fcell_buf, FCELL_TYPE);
		for (j = 1; j < ncols + 1; j++) {
		    *(cor_fcell_buf + j - 1) = (float)(*(*(cor + i) + j));
		}

		if (G_put_raster_row(fe, cor_fcell_buf, FCELL_TYPE) < 0)
		    exit(EXIT_FAILURE);
	    }
	    break;
	case DCELL_TYPE:
	    cor_dcell_buf = G_allocate_raster_buf(DCELL_TYPE);
	    fe = G_open_raster_new("interior", DCELL_TYPE);
	    for (i = 1; i < nrows + 1; i++) {
		G_zero_raster_buf(cor_dcell_buf, DCELL_TYPE);
		for (j = 1; j < ncols + 1; j++)
		    *(cor_dcell_buf + j - 1) = (double)(*(*(cor + i) + j));

		if (G_put_raster_row(fe, cor_dcell_buf, DCELL_TYPE) < 0)
		    exit(EXIT_FAILURE);
	    }
	    break;
	}
    }


    /* if a map of patch numbers was requested,
       complete the details of map creation */

    if (choice->patchmap) {
	fd = G_open_raster_new("num", CELL_TYPE);
	for (i = 1; i < nrows + 1; i++) {
	    pat_buf = G_allocate_raster_buf(CELL_TYPE);
	    G_zero_raster_buf(pat_buf, CELL_TYPE);
	    for (j = 1; j < ncols + 1; j++)
		*(pat_buf + j - 1) = *(*(pat + i) + j);

	    if (G_put_raster_row(fd, pat_buf, CELL_TYPE) < 0)
		exit(EXIT_FAILURE);
	}
    }

    /* print out the patch list */

    /*  printf("\nPatch list after tracing\n");
       list_head = patch_list;
       while(list_head) {
       printf("num=%d att=%f pts=%d long=%3.0f crow=%3.0f ccol=%3.0f n=%d s=%d
       e=%d w=%d area=%3.0f per=%3.0f core=%3.0f edge=%3.0f\n",list_head->num,
       list_head->att,
       list_head->npts, list_head->long_axis, list_head->c_row,
       list_head->c_col, list_head->n,list_head->s,list_head->e,list_head->w,
       list_head->area, list_head->perim, list_head->core, list_head->edge);
       for (i=0; i<list_head->npts; i++) {
       printf("point[%d]   row=%d col=%d\n",i,*(list_head->row +
       i),*(list_head->col + i));
       }
       list_head = list_head->next;
       }
     */

    /* if moving window method selected, then
       call the moving window driver program */


    if (choice->wrum == 'm') {
	if (!empty) {
	    if (!centernull)
		mv_patch(patch_list, value, index);
	    else {
		for (p = 0; p < 42; p++)
		    *(*(value + index) + p) = -BIG;
	    }
	}
	else {
	    for (p = 0; p < 42; p++)
		*(*(value + index) + p) = -BIG;
	}
    }

    /* otherwise call the other driver program */

    else {
	if (!empty)
	    df_patch(patch_list);
    }

    /* free memory allocated for content buffer */

    for (i = 0; i < nrows + 3; i++)
	G_free(buf[i]);
    G_free(buf);

    /* free memory allocated for null buffer */

    for (i = 0; i < nrows + 3; i++)
	G_free(null_buf[i]);
    G_free(null_buf);

    /* free memory allocated for num map */

    if (choice->patchmap) {
	for (i = 0; i < nrows + 3; i++)
	    G_free(pat[i]);
	G_free(pat);
    }

    /* free memory allocated for core map */

    if (choice->coremap) {
	for (i = 0; i < nrows + 3; i++)
	    G_free(cor[i]);
	G_free(cor);
    }

    /* free memory allocated for patch list */

    if (total_patches) {
	list_head = patch_list;
	while (list_head) {
	    G_free(list_head->col);
	    G_free(list_head->row);
	    G_free(list_head);
	    list_head = list_head->next;
	}
    }

    /* close the num file and release the
       memory allocated for it */

    if (choice->patchmap) {
	G_close_cell(fd);
	G_free(pat_buf);
    }

    /* close the core file, copy the category
       information, the statistics, the color
       table, and the history over to map 
       "interior" and then release the
       memory allocated for the cor_buf */

    if (choice->coremap) {
	G_close_cell(fe);
	G_rewind_cell_stats(&stats);
	G_rewind_raster_cats(&cats);

	if (cats_ok && data_type == CELL_TYPE) {
	    long count;
	    void *rast1, *rast2;

	    rast1 = cor_cell_buf;
	    rast2 = G_incr_void_ptr(rast1, G_raster_size(CELL_TYPE));
	    while (G_next_cell_stat(rast1, &count, &stats))
		G_set_raster_cat(rast1, rast2, G_get_raster_cat(rast1, &cats,
								CELL_TYPE),
				 &newcats, CELL_TYPE);
	    G_write_raster_cats("interior", &newcats);
	    G_free_raster_cats(&cats);
	    G_free_raster_cats(&newcats);
	    G_free_cell_stats(&stats);
	}

	if (colr_ok) {
	    if (data_type == CELL_TYPE) {
		CELL min, max, cmin, cmax;

		G_read_range("interior", mapset, &range);
		G_get_range_min_max(&range, &min, &max);
		G_get_color_range(&cmin, &cmax, &colr);
		if (min > cmin)
		    cmin = min;
		if (max < cmax)
		    cmax = max;
		G_set_color_range(cmin, cmax, &colr);
	    }
	    else {
		DCELL dmin, dmax;
		CELL cmin, cmax;

		G_read_fp_range("interior", mapset, &fprange);
		G_get_fp_range_min_max(&fprange, &dmin, &dmax);
		G_get_color_range(&cmin, &cmax, &colr);
		if (dmin > cmin)
		    cmin = dmin;
		if (dmax < cmax)
		    cmax = dmax;
		G_set_color_range(cmin, cmax, &colr);
	    }
	    G_write_colors("interior", mapset, &colr);
	}

	if (range_ok) {
	    if (data_type == CELL_TYPE)
		G_write_range("interior", &range);
	    else
		G_write_fp_range("interior", &fprange);
	}

	if (hist_ok)
	    G_write_history("interior", &hist);

	G_free_cats(&cats);
	G_free_colors(&colr);
	switch (data_type) {
	case CELL_TYPE:
	    G_free(cor_cell_buf);
	    break;
	case FCELL_TYPE:
	    G_free(cor_fcell_buf);
	    break;
	case DCELL_TYPE:
	    G_free(cor_dcell_buf);
	    break;
	}
    }
    return;
}






				/* OPEN THE RASTER FILE TO BE CLIPPED,
				   AND DO THE CLIPPING */

void cell_clip(DCELL ** buf, DCELL ** null_buf, int row0, int col0, int nrows,
	       int ncols, int index, float radius, int *centernull,
	       int *empty)
{
    CELL *tmp, *tmp1;
    FCELL *ftmp;
    DCELL *dtmp;
    void *rastptr;
    char *tmpname;
    int fr, x;
    register int i, j;
    double center_row = 0.0, center_col = 0.0;
    double dist;
    RASTER_MAP_TYPE data_type;

    /*
       Variables:
       IN:
       buf =        pointer to array containing only the pixels inside the area
       that was specified to be clipped, so a smaller array than the
       original raster map
       null_buf =   pointer to array containing 0.0 if pixel in input raster map is
       not null and 1.0 if pixel in input raster map is null
       row0 =       starting row for the area to be clipped out of the raster map
       col0 =       starting col for the area to be clipped out of the raster map
       nrows =      total number of rows in the area to be clipped
       ncols =      total number of cols in the area to be clipped
       index =      number of the region to be clipped, if there's a region map
       radius =     radius of the circle to be clipped, if circles chosen for
       sampling units
       centernull = 1 if the center pixel of the clipped area is a null value,
       0 otherwise
       empty =      1 is the whole clipped area contains null values, 0 otherwise.
       INTERNAL:
       tmp =        pointer to a temporary buffer to store a row of a CELL_TYPE
       (integer) raster
       ftmp =       pointer to a temporary buffer to store a row of an FCELL_TYPE
       (floating point) raster
       dtmp =       pointer to a temporary buffer to store a row of a DCELL_TYPE
       (double) raster
       tmp1 =       pointer to a temporary buffer to store a row of the region map
       tmpname =    one of the above: tmp, ftmp, dtmp
       fr =         return value from attempting to open the region map
       i, j =       indices to rows and cols of the arrays
       center_row = row of the center of the circle, if circles used
       center_col = column of the center of the circle, if circles used
       dist =       used to measure distance from a row/column to the center of the
       circle, to see if a row/column is within the circle
       data_type =  the type of raster map: integer, floating point, or double
       rastptr =    void pointer used to advance through null values in the tmp,
       ftmp, or dtmp buffers

     */

    /* if sampling by region was chosen, check
       for the region map and make sure it is
       an integer (CELL_TYPE) map */

    if (choice->wrum == 'r') {
	if (0 > (fr = G_open_cell_old(choice->reg, G_mapset()))) {
	    fprintf(stderr, "\n");
	    fprintf(stderr,
		    "   *******************************************************\n");
	    fprintf(stderr,
		    "    You specified sam=r to request sampling by region,    \n");
	    fprintf(stderr,
		    "    but the region map specified with the 'reg=' parameter\n");
	    fprintf(stderr,
		    "    cannot be found in the current mapset.                \n");
	    fprintf(stderr,
		    "   *******************************************************\n");
	    exit(EXIT_FAILURE);
	}
	if (G_raster_map_type(choice->reg, G_mapset()) > 0) {
	    fprintf(stderr, "\n");
	    fprintf(stderr,
		    "   *******************************************************\n");
	    fprintf(stderr,
		    "    You specified sam=r to request sampling by region,    \n");
	    fprintf(stderr,
		    "    but the region map specified with the 'reg=' parameter\n");
	    fprintf(stderr,
		    "    must be an integer map, and it is floating point or   \n");
	    fprintf(stderr,
		    "    double instead.                                       \n");
	    fprintf(stderr,
		    "   *******************************************************\n");
	    exit(EXIT_FAILURE);
	}
	tmp1 = G_allocate_raster_buf(CELL_TYPE);
	G_zero_raster_buf(tmp1, CELL_TYPE);
	fprintf(stderr, "Analyzing region number %d...\n", index);
    }

    data_type = G_raster_map_type(choice->fn, G_mapset());


    /* allocate memory to store a row of the
       raster map, depending on the type of
       input raster map; keep track of the
       name of the buffer for each raster type */

    switch (data_type) {
    case CELL_TYPE:
	tmp = G_allocate_raster_buf(CELL_TYPE);
	tmpname = "tmp";
	break;
    case FCELL_TYPE:
	ftmp = G_allocate_raster_buf(FCELL_TYPE);
	tmpname = "ftmp";
	break;
    case DCELL_TYPE:
	dtmp = G_allocate_raster_buf(DCELL_TYPE);
	tmpname = "dtmp";
	break;
    }

    /* zero the buffer used to hold null values */

    for (i = 0; i < nrows; i++) {
	for (j = 0; j < ncols; j++) {
	    null_buf[i][j] = 1.0;
	}
    }

    /* if circles are used for sampling, then
       calculate the center of the area to be
       clipped, in pixels */

    if ((int)radius) {
	center_row = ((double)row0 + ((double)nrows - 1) / 2);
	center_col = ((double)col0 + ((double)ncols - 1) / 2);
    }

    /* for each row of the area to be clipped */

    /*printf("\n\nNEW WINDOW\n"); */


    for (i = row0; i < row0 + nrows; i++) {

	/* if region, read in the corresponding
	   map row in the region file */

	if (choice->wrum == 'r')
	    G_get_raster_row_nomask(fr, tmp1, i, CELL_TYPE);

	/* initialize each element of the
	   row buffer to 0; this row buffer
	   will hold one row of the clipped
	   raster map.  Then read row i of the
	   map into tmp buffer */

	switch (data_type) {
	case CELL_TYPE:
	    G_zero_raster_buf(tmp, data_type);
	    G_get_raster_row(finput, tmp, i, data_type);
	    break;
	case FCELL_TYPE:
	    G_zero_raster_buf(ftmp, data_type);
	    G_get_raster_row(finput, ftmp, i, data_type);
	    break;
	case DCELL_TYPE:
	    G_zero_raster_buf(dtmp, data_type);
	    G_get_raster_row(finput, dtmp, i, data_type);
	    break;
	}


	/* set up a void pointer to be used to find null
	   values in the area to be clipped and advance
	   the raster pointer to the right starting
	   column */

	switch (data_type) {
	case CELL_TYPE:
	    rastptr = tmp;
	    for (x = 0; x < col0; x++)
		rastptr = G_incr_void_ptr(rastptr, G_raster_size(CELL_TYPE));
	    break;
	case FCELL_TYPE:
	    rastptr = ftmp;
	    for (x = 0; x < col0; x++)
		rastptr = G_incr_void_ptr(rastptr, G_raster_size(FCELL_TYPE));
	    break;
	case DCELL_TYPE:
	    rastptr = dtmp;
	    for (x = 0; x < col0; x++)
		rastptr = G_incr_void_ptr(rastptr, G_raster_size(DCELL_TYPE));
	    break;
	}


	/* for all the columns one by one */

	for (j = col0; j < col0 + ncols; j++) {

	    /* look for null values in each cell
	       and set centernull flag to 1 if
	       center is null and empty to 1 if
	       no cells are found that are not null */

	    switch (data_type) {
	    case CELL_TYPE:
		if (G_is_null_value(rastptr, CELL_TYPE)) {
		    *(*(null_buf + i + 1 - row0) + j + 1 - col0) = 1.0;
		    if (i == row0 + nrows / 2 && j == col0 + ncols / 2)
			*centernull = 1;
		}
		else {
		    *empty = 0;
		    if (choice->wrum != 'r' || *(tmp1 + j) == index)
			*(*(null_buf + i + 1 - row0) + j + 1 - col0) = 0.0;
		    else
			*(*(null_buf + i + 1 - row0) + j + 1 - col0) = 1.0;
		}
		rastptr = G_incr_void_ptr(rastptr, G_raster_size(CELL_TYPE));
		break;

	    case FCELL_TYPE:
		if (G_is_null_value(rastptr, FCELL_TYPE)) {
		    *(*(null_buf + i + 1 - row0) + j + 1 - col0) = 1.0;
		    if (i == row0 + nrows / 2 && j == col0 + ncols / 2)
			*centernull = 1;
		}
		else {
		    *empty = 0;
		    if (choice->wrum != 'r' || *(tmp1 + j) == index)
			*(*(null_buf + i + 1 - row0) + j + 1 - col0) = 0.0;
		    else
			*(*(null_buf + i + 1 - row0) + j + 1 - col0) = 1.0;
		}
		rastptr = G_incr_void_ptr(rastptr, G_raster_size(FCELL_TYPE));
		break;

	    case DCELL_TYPE:
		if (G_is_null_value(rastptr, DCELL_TYPE)) {
		    *(*(null_buf + i + 1 - row0) + j + 1 - col0) = 1.0;
		    if (i == row0 + nrows / 2 && j == col0 + ncols / 2)
			*centernull = 1;
		}
		else {
		    *empty = 0;
		    if (choice->wrum != 'r' || *(tmp1 + j) == index)
			*(*(null_buf + i + 1 - row0) + j + 1 - col0) = 0.0;
		    else
			*(*(null_buf + i + 1 - row0) + j + 1 - col0) = 1.0;
		}
		rastptr = G_incr_void_ptr(rastptr, G_raster_size(CELL_TYPE));
		break;
	    }

	    /* if circles are used for sampling */

	    if ((int)radius) {
		dist =
		    sqrt(((double)i - center_row) * ((double)i - center_row) +
			 ((double)j - center_col) * ((double)j - center_col));

		/* copy the contents of the correct tmp file
		   into the appropriate cell in the buf */

		if (dist < radius) {
		    switch (data_type) {
		    case CELL_TYPE:
			*(*(buf + i + 1 - row0) + j + 1 - col0) = *(tmp + j);
			break;
		    case FCELL_TYPE:
			*(*(buf + i + 1 - row0) + j + 1 - col0) = *(ftmp + j);
			break;
		    case DCELL_TYPE:
			*(*(buf + i + 1 - row0) + j + 1 - col0) = *(dtmp + j);
			break;
		    }
		}
		else
		    *(*(null_buf + i + 1 - row0) + j + 1 - col0) = 1.0;
	    }

	    /* if circles are not used and
	       if the choice is not "by region" or
	       if this column is in region "index" */

	    else if (choice->wrum != 'r' || *(tmp1 + j) == index) {

		/* copy the contents of the correct tmp
		   into the appropriate cell in the buf
		   and the corresponding null values into
		   the appropriate cell in null_buf */

		switch (data_type) {
		case CELL_TYPE:
		    *(*(buf + i + 1 - row0) + j + 1 - col0) = *(tmp + j);
		    break;
		case FCELL_TYPE:
		    *(*(buf + i + 1 - row0) + j + 1 - col0) = *(ftmp + j);
		    break;
		case DCELL_TYPE:
		    *(*(buf + i + 1 - row0) + j + 1 - col0) = *(dtmp + j);
		    break;
		}

		/*printf("cell_clip i=%d j=%d buf= %12.0f null_buf = %d\n", i, j,
		 *(*(buf +i+1-row0)+j+1-col0), *(*(null_buf +i +1-row0) +j +1 -col0)); */

	    }
	}
    }

    switch (data_type) {
    case CELL_TYPE:
	G_free(tmp);
	break;
    case FCELL_TYPE:
	G_free(ftmp);
	break;
    case DCELL_TYPE:
	G_free(dtmp);
	break;
    }
    if (choice->wrum == 'r') {
	G_free(tmp1);
	G_close_cell(fr);
    }
    return;
}






				/* DRIVER TO LOOK FOR NEW PATCHES, CALL
				   THE TRACING ROUTINE, AND ADD NEW PATCHES
				   TO THE PATCH LIST */

void trace(int nrows, int ncols, DCELL ** buf, DCELL ** null_buf, CELL ** pat,
	   DCELL ** cor)
{
    double class = 0.0;
    register int i, j;
    PATCH *tmp, *find_any, *list_head;

    /*
       Variables:
       EXTERN:
       IN:
       nrows =      total number of rows in the area where tracing will occur
       ncols =      total number of cols in the area where tracing will occur
       buf =        pointer to array containing only the pixels inside the area
       that was clipped and within which tracing will now occur, so
       a smaller array than the original raster map
       null_buf =   pointer to array containing 0.0 if pixel in input raster map is
       not null and 1.0 if pixel in input raster map is null
       pat =        pointer to array containing the map of patch numbers; this map
       can only be integer
       cor =        pointer to array containing the map of interior area; this map
       can be integer, floating point, or double, but is stored as a
       double throughout the program
       INTERNAL:
       class =      the attribute of each pixel
       i, j =       counts the row and column as the program goes through the area
       tmp =        pointer to a member of the PATCH list data structure, used to
       advance through the patch list
       find_any =   pointer to a member of the patch list to hold the results after
       routine get_bd is called to trace the boundary of the patch and
       save the patch information in the PATCH data structure
       list_head =  pointer to the first member of the patch list
     */

    /* go thru buf, which contains the entries
       within the clipped window, column by
       column and row by row; Note that now the
       rows and cols are counted from 1 to nrows
       and 1 to ncols, not from 0 to < nrows and
       0 to < ncols */

    i = 0;
    while (i++ < nrows) {	/*1 */
	j = 0;
	while (j++ < ncols) {

	    /* if this pt contains a positive or negative
	       raster value or a zero, and null value is
	       0.0, it may be the start of an untraced
	       patch */

	    if ((*(*(buf + i) + j) || *(*(buf + i) + j) == 0.0) && *(*(null_buf + i) + j) == 0.0) {	/*3 */
		class = *(*(buf + i) + j);

		/* trace the patch from the current pt */

		list_head = patch_list;

		if ((find_any = get_bd(i, j, nrows, ncols, class, buf, null_buf, list_head, pat, cor))) {	/*4 */

		    /* if the first patch, make tmp point to
		       the patch list and add the first patch
		       to the list */

		    if (total_patches == 0) {
			patch_list = find_any;
			tmp = patch_list;
		    }
		    /* add the next patch to the patch list */

		    else {
			tmp->next = find_any;
			tmp = tmp->next;
		    }

		    /* increment the count of total patches */

		    total_patches++;
		}		/*4 */

		/* if i and j are now at or outside the
		   limits of the window, then quit */

		if (i >= nrows && j >= ncols) {
		    return;
		}
	    }			/*3 */

	    /* if this pt is the boundary point of an
	       already traced patch or is outside the
	       window, do not start tracing; skip to
	       next pt */

	    else {		/*5 */

		/* if i and j are now at or outside the
		   limits of the window, then quit */

		if (i >= nrows && j >= ncols)
		    return;

	    }			/*5 */
	}			/*2 */
    }
    return;			/*1 */
}







				/* TRACE THE BOUNDARY OF A PATCH AND
				   SAVE THE PATCH CHARACTERISTICS IN
				   THE PATCH STRUCTURE */

PATCH *get_bd(int row0, int col0, int nrows, int ncols, double class,
	      DCELL ** buf, DCELL ** null_buf, PATCH * p_list, CELL ** pat,
	      DCELL ** cor)
{
    int i = row0, j = col0, pts = 0, di = 0, dj = -1,
	not_done, k, m, tmp, lng = 0, roww = 0, rowe = 0,
	row1, col1, di2, dj2, p, q, area, per, corearea, edgearea, nozero;
    int ***twist2, trows, tcols, n, e, a, b;
    float ***twistP, sumT;
    PATCH *patch;
    CELL **patchmap;
    PT *ptrfirst, *ptrthis, *ptrnew, *ptrfree;


    /*
       Variables:
       IN:
       row0 =        row for the first pixel in the patch
       col0 =        column for the first pixel in the patch
       nrows =       number of rows in the clipped area
       ncols =       number of columns in the clipped area
       class =       attribute of the patch being traced
       buf =         pointer to array containing only the pixels inside the area
       that was clipped
       null_buf =    pointer to array containing 0.0 if pixel in input raster map is
       not null and 1.0 if pixel in input raster map is null
       p_list =      pointer to the patch list
       pat =         pointer to array containing the map of patch numbers; this map
       can only be integer
       cor =         pointer to array containing the map of interior area; this map
       can be integer, floating point, or double, but is stored as a
       double throughout the program
       INTERNAL
       i, j =        counts the row and column as tracing occurs
       pts =         counts the number of points as tracing proceeds
       di, dj =      indicates moves of 1 pixel in the row or column direction, used
       to examine adjoining 4- or 8-neighboring pixels when tracing
       not_done =    flag to indicate when the patch has not been fully traced yet
       k =
       m =
       tmp =         integer to hold intermediate results in calculation of long axis
       of patch
       lng =         integer to hold the calculated long axis of the patch
       roww =        integer to hold the westernmost point in each row of the patch
       rowe =        integer to hold the easternmost point in each row of the patch
       coln =        integer to hold the northernmost point in each row of the patch
       cols =        integer to hold the southernmost point in each row of the patch
       row1 =        integer to hold the starting row for tracing internal boundaries
       col1 =        integer to hold the starting column for tracing internal boundaries
       di2, dj2 =    indicates moves of 1 pixel in the row or column direction, used
       to examine adjoining 4- or 8-neighboring pixels when tracing
       internal boundaries of a patch
       p, q =        row and column indices for tracing patch internal boundaries
       area =        patch area
       per =         patch perimeter
       corearea =    patch interior area
       edgearea =    patch edge area
       nozero =
       twist2 =      2-dimensional array to hold preliminary countes needed to calculate
       the twist number and omega index
       trows =       number of rows in the bounding box for the patch
       tcols =       number of columns in the bounding box for the patch
       n, e =        northing (in rows) and easting (in columns) for the patch
       a, b =        indexes particular pixels used in calculating twist number in certain
       cases
       twistnum =    twist number
       twistP =      2-dimensional array to hold P values used in the calculation of twist
       number
       sumT =        the floating point version of twist number
       omega =       omega index
       patch =       pointer to a member of the patch list
       patchmap =    2-dimensional array holding 1's for pixels inside the patch and
       zero otherwise
       ptrfirst =    pointer to the first element of the linked list of patches
       ptrthis =     pointer to the current element of the linked list of patches
       ptrnew =      pointer to a new element of the linked list of patches
       ptrfree =
     */



    /* allocate memory for 1 patch to be
       saved in the patch data structure */

    patch = (PATCH *) G_calloc(1, sizeof(PATCH));

    /* allocate memory for patchmap, which
       will hold the patch boundaries found in
       the buf array */

    patchmap = (CELL **) G_calloc(nrows + 3, sizeof(CELL *));
    for (m = 0; m < nrows + 3; m++)
	patchmap[m] = (CELL *) G_allocate_raster_buf(CELL_TYPE);

    /* print on the screen a message indicating
       that tracing has reached a certain patch */

    if (choice->wrum != 'm') {
	fprintf(stderr, "Tracing patch %7d\r", total_patches + 1);
    }


    /* if this is the first patch to be traced,
       then set the next patch on the patch list
       to NULL */

    if (total_patches == 0)
	patch->next = (PATCH *) NULL;

    /* this loop goes until the patch has been 
       traced */
    for (;;) {			/*1 */


	/* STEP 1: RECORD ATTRIBUTE AND PATCH NUMBER,
	   THEN TRACE THE PTS IN THE BOUNDARY, 
	   RECORDING THE ROW AND COL OF EACH PT, AND
	   FINDING THE PATCH BOUNDING BOX */

	/* initialize variables */

	not_done = 1;
	patch->s = 0;
	patch->e = 0;
	patch->w = (int)BIG;
	patch->n = (int)BIG;

	/* while tracing is not done */

	while (not_done) {	/*2 */


	    /* if this is the first pt in the patch,
	       fill the PATCH structure with the 
	       attribute and number of the patch,
	       and set the first pt to NULL */

	    if (pts == 0) {
		patch->att = class;
		patch->num = total_patches + 1;
		ptrfirst = (PT *) NULL;
	    }

	    /* if this pt has a non-null value and
	       it hasn't been traced.
	       (1) put a 1 in patchmap at the location.
	       This will keep track of all the pixels
	       that get traced in this one patch.
	       (2) make the null_buf value a 1.0.  This
	       will keep track of all the pixels that
	       get traced or are otherwise null in all
	       the patches in the buffer.  Once they
	       are null, they don't get traced again.
	       (3) save the row & col in PATCH structure,
	       (4) see if the pt expands the current
	       bounding box
	       (5) increment the pts count */

	    /*printf("num=%d i=%d j=%d buf=%f patchmap=%d null=%f START GET_BD\n",
	       patch->num,i,j,*(*(buf + i) + j),*(*(patchmap + i) + j), *(*(null_buf + i) + 
	       j)); */


	    if ((*(*(buf + i) + j) ||
		 *(*(buf + i) + j) == 0.0) &&
		*(*(patchmap + i) + j) == 0 &&
		*(*(null_buf + i) + j) == 0.0) {

		*(*(patchmap + i) + j) = 1;
		*(*(null_buf + i) + j) = 1.0;
		ptrnew = (PT *) G_calloc(1, sizeof(PT));
		if (ptrfirst == (PT *) NULL) {
		    ptrfirst = ptrthis = ptrnew;
		}
		else {
		    ptrthis = ptrfirst;
		    while (ptrthis->next != (PT *) NULL)
			ptrthis = ptrthis->next;
		    ptrthis->next = ptrnew;
		    ptrthis = ptrnew;
		}
		ptrthis->row = i;
		ptrthis->col = j;
		if (i > patch->s)
		    patch->s = i;
		if (i < patch->n)
		    patch->n = i;
		if (j > patch->e)
		    patch->e = j;
		if (j < patch->w)
		    patch->w = j;
		pts++;
	    }

	    /*printf("3. i=%d j=%d s=%d n=%d w=%d e=%d\n",i,j, patch->s, patch->n,
	       patch->w,patch->e); */



	    /* if there is a neighboring pixel, with the
	       same class, moving clockwise around the 
	       patch, then reset i and j to this
	       location, then reset di and dj */

	    if (yes_nb(&di, &dj, buf, class, i, j, nrows, ncols)) {
		i = i + di;
		j = j + dj;
		di = -di;
		dj = -dj;
		clockwise(&di, &dj);

		/*printf("num=%d i=%d j=%d buf=%f patchmap=%d null=%d row0=%d col0=%d \
		   AFTER YES_NB\n",
		   patch->num,i,j, *(*(buf + i) + j), *(*(patchmap + i) + j), 
		   *(*(null_buf + i) + j), row0, col0); 
		 */

		/* if tracing has returned to the starting
		   pt, then stop; in a special case with
		   diagonal tracing, don't stop if there is
		   a traceable pixel below and to the left
		   and if there is a below and to the left */

		if (i == row0 && j == col0) {
		    not_done = 0;
		    if (choice->trace &&
			(i < nrows) && (j > 1) &&
			*(*(buf + i + 1) + j - 1) == class &&
			(*(*(patchmap + i + 1) + j - 1) == 0) &&
			(*(*(null_buf + i + 1) + j - 1) == 0.0)) {

			/*printf("IN NOT DONE i=%d j=%d i=%d j=%d buf=%f patchmap=%d null_buf=%f\n",
			   i,j,i+1,j-1,*(*(buf + i + 1) + j - 1),*(*(patchmap + i + 1) + j - 1),
			   *(*(null_buf + i + 1) + j - 1)); */

			not_done = 1;
		    }
		}
	    }

	    /* if there is no neighboring pixel with the
	       same class, then stop tracing */

	    else
		not_done = 0;

	}			/*2 */


	/* STEP 2: CLEAN AND FILL THE PATCH WITHIN
	   ITS BOUNDARIES. THE MAP IS CLEANED AND 
	   FILLED INTO "PATCHMAP" WHICH THEN CONTAINS
	   THE FOLLOWING VALUES:
	   1 = BOUNDARY PT
	   -999 = INTERIOR (NON BOUNDARY) PT */

	for (i = patch->n; i < patch->s + 1; i++) {	/*3 */

	    /* find the westernmost and easternmost boundary
	       points in row i */

	    roww = patch->w;
	    rowe = patch->e;
	    while (*(*(patchmap + i) + roww) == 0 && roww < patch->e)
		roww++;
	    while (*(*(patchmap + i) + rowe) == 0 && rowe > patch->w)
		rowe--;

	    /* if the westernmost and easternmost boundary
	       pts in row i are not the same or are not
	       next to each other, then we need to scan
	       across row i */

	    if (roww != rowe && roww + 1 != rowe) {	/*4 */
		for (j = roww; j < rowe; j++) {	/*5 */

		    /* if this pixel is one of the traced boundary 
		       or interior pts and the next pixel in the row
		       is not one of these or has not been traced, */

		    if (*(*(patchmap + i) + j) != 0 && *(*(patchmap + i) + j + 1) == 0) {	/*6 */

			/* if the next pixel has the same class, then 
			   give that pixel a -999 in patchmap to signify 
			   that it is part of the patch, and make null_buf 
			   a 1.0 to signify that this next pixel has been 
			   traced */

			if (*(*(buf + i) + j + 1) == class) {
			    *(*(patchmap + i) + j + 1) = -999;
			    *(*(null_buf + i) + j + 1) = 1.0;
			}

			/* but if the next pixel doesn't have the same 
			   class, then the present pixel marks the edge 
			   of a potential interior boundary for the patch.  
			   Trace this boundary only if it has not already 
			   been traced */

			else if (*(*(buf + i) + j + 1) != class && (*(*(patchmap + i) + j) != 1 || *(*(patchmap + i) + j + 1) == 0)) {	/*7 */
			    not_done = 1;
			    row1 = p = i;
			    col1 = q = j;
			    di2 = 0;
			    dj2 = 1;
			    while (not_done) {	/*8 */
				if (*(*(patchmap + p) + q) == -999)
				    *(*(patchmap + p) + q) = 4;
				if (*(*(patchmap + p) + q) == 4) {
				    ptrnew = (PT *) G_calloc(1, sizeof(PT));
				    ptrthis = ptrfirst;
				    while (ptrthis->next != (PT *) NULL)
					ptrthis = ptrthis->next;
				    ptrthis->next = ptrnew;
				    ptrthis = ptrnew;
				    ptrthis->row = p;
				    ptrthis->col = q;
				    *(*(patchmap + p) + q) = 1;
				    *(*(null_buf + p) + q) = 1.0;
				    pts++;
				}
				if (yes_nb(&di2, &dj2, buf, class, p, q,
					   nrows, ncols)) {
				    p = p + di2;
				    q = q + dj2;
				    if (*(*(patchmap + p) + q) != 1) {
					*(*(patchmap + p) + q) = 4;
					*(*(null_buf + p) + q) = 1.0;
				    }
				    di2 = -di2;
				    dj2 = -dj2;
				    clockwise(&di2, &dj2);
				    if (p == row1 && q == col1) {
					not_done = 0;
				    }
				}
				else
				    not_done = 0;
			    }	/*8 */
			}	/*7 */
		    }		/*6 */
		}		/*5 */
	    }			/*4 */
	}			/*3 */




	/* STEP 3: GO THROUGH THE RESULTING PATCHMAP
	   AND FIND THE INTERIOR & EDGE AREA IF REQUESTED */

	if (choice->core[0]) {
	    for (k = 0; k < choice->edge; k++) {
		for (i = patch->n; i < patch->s + 1; i++) {
		    for (j = patch->w; j < patch->e + 1; j++) {
			if ((k > 0 && *(*(patchmap + i) + j) == k) ||
			    (k == 0 && *(*(patchmap + i) + j) == 1)) {

			    /* if the sampling area border is not to
			       be considered patch edge and we're
			       interior of the sampling area border,
			       then we can search for interior; OR if the
			       sampling area border is to be considered
			       patch edge, then we can search for interior */

			    if ((choice->perim2 && i != 1 && i != nrows &&
				 j != 1 && j != ncols) || !choice->perim2) {
				di = 0;
				dj = -1;
				for (m = 0; m < 8; m++) {
				    if (*(*(patchmap + i + di) + j + dj) ==
					-999) {
					if (choice->trace) {
					    if (k > 0)
						*(*(patchmap + i + di) + j +
						  dj) = k + 1;
					}
					else if (di == 0 || dj == 0) {
					    if (k > 0)
						*(*(patchmap + i + di) + j +
						  dj) = k + 1;
					}
				    }
				    clockwise(&di, &dj);
				}
			    }
			    else {
				nozero = 1;
				if (j != 1)
				    if (*(*(patchmap + i) + j - 1) == 0)
					nozero = 0;
				if (i != 1 && j != 1)
				    if (*(*(patchmap + i - 1) + j - 1) == 0)
					nozero = 0;
				if (i != 1)
				    if (*(*(patchmap + i - 1) + j) == 0)
					nozero = 0;
				if (i != 1 && j != ncols)
				    if (*(*(patchmap + i - 1) + j + 1) == 0)
					nozero = 0;
				if (j != ncols)
				    if (*(*(patchmap + i) + j + 1) == 0)
					nozero = 0;
				if (i != nrows && j != ncols)
				    if (*(*(patchmap + i + 1) + j + 1) == 0)
					nozero = 0;
				if (i != nrows)
				    if (*(*(patchmap + i + 1) + j) == 0)
					nozero = 0;
				if (i != nrows && j != 1)
				    if (*(*(patchmap + i + 1) + j - 1) == 0)
					nozero = 0;
				if (nozero)
				    *(*(patchmap + i) + j) = -999;
			    }
			}
		    }
		}
	    }
	}



	/* STEP 4: GO THROUGH THE RESULTING PATCHMAP AND DETERMINE 
	   THE PATCH SIZE, AMOUNT OF PERIMETER AND, IF REQUESTED, 
	   THE CORE SIZE AND EDGE SIZE */

	area = 0;
	per = 0;
	corearea = 0;
	edgearea = 0;
	for (i = patch->n; i < patch->s + 1; i++) {
	    for (j = patch->w; j < patch->e + 1; j++) {
		if (*(*(patchmap + i) + j) || *(*(patchmap + i) + j) == -999) {
		    area++;
		    if (choice->perim2 == 0) {
			if (j == 1 || j == ncols)
			    per++;
		    }
		    if (j < ncols && *(*(patchmap + i) + j + 1) == 0)
			per++;
		    if (j > 1 && *(*(patchmap + i) + j - 1) == 0)
			per++;

		    /* if a num map was requested with the -n flag,
		       then copy the patch numbers into pat array */

		    if (choice->patchmap)
			*(*(pat + i) + j) = patch->num;

		    /* if core calculations are requested */

		    if (choice->core[0]) {
			if (*(*(patchmap + i) + j) == -999)
			    corearea++;
			if (*(*(patchmap + i) + j) > 0)
			    edgearea++;
		    }

		    /* if core map is requested */

		    if (choice->coremap) {
			if (*(*(patchmap + i) + j) == -999)
			    *(*(cor + i) + j) = *(*(buf + i) + j);
		    }
		}
	    }
	}
	for (j = patch->w; j < patch->e + 1; j++) {
	    for (i = patch->n; i < patch->s + 1; i++) {
		if (*(*(patchmap + i) + j) || *(*(patchmap + i) + j) == -999) {
		    if (choice->perim2 == 0) {
			if (i == 1 || i == nrows)
			    per++;
		    }
		    if (i < nrows && *(*(patchmap + i + 1) + j) == 0)
			per++;
		    if (i > 1 && *(*(patchmap + i - 1) + j) == 0)
			per++;
		}
	    }
	}
	patch->area = area;
	patch->perim = per;
	patch->edge = edgearea;
	patch->core = corearea;



	/* STEP 5: GO THROUGH THE RESULTING LIST OF PTS,
	   RECORD THE ROW AND COL IN THE PATCH 
	   STRUCTURE, AND FIND THE LONG AXIS AND
	   CENTER OF THE PATCH */

	patch->npts = pts;

	/* allocate enough memory to store the list of
	   pts in the PATCH structure */

	patch->col = (int *)G_calloc(pts, sizeof(int));
	patch->row = (int *)G_calloc(pts, sizeof(int));

	/* go through the list of pts */

	i = 0;
	ptrthis = ptrfirst;
	while (ptrthis) {
	    ptrfree = ptrthis;

	    /* save the pt locat. in the PATCH structure */

	    *(patch->row + i) = ptrthis->row;
	    *(patch->col + i) = ptrthis->col;

	    /* long-axis step 1: find the largest
	       sum of squares between patch boundary
	       pts if the Related Circumscribing Circle
	       shape index is requested */

	    if (choice->Mx[3]) {
		if (pts == 1) {
		    lng = 2;
		}
		else {
		    for (j = 0; j < i + 1; j++) {
			if ((tmp =
			     (abs(*(patch->row + j) - *(patch->row + i)) +
			      1) * (abs(*(patch->row + j) -
					*(patch->row + i)) + 1) +
			     (abs(*(patch->col + j) - *(patch->col + i)) +
			      1) * (abs(*(patch->col + j) -
					*(patch->col + i)) + 1)) > lng)
			    lng = tmp;
		    }
		}
	    }
	    /* patch center step 1: sum up the boundary
	       coordinates */

	    if (i < pts) {
		patch->c_row += *(patch->row + i);
		patch->c_col += *(patch->col + i);
	    }

	    ptrthis = ptrthis->next;
	    G_free(ptrfree);
	    i++;
	}

	/* patch long axis and center step 2: complete
	   the calculations */

	if (choice->Mx[3])
	    patch->long_axis = sqrt((double)(lng));
	patch->c_col = (int)(patch->c_col / pts + 0.5);
	patch->c_row = (int)(patch->c_row / pts + 0.5);




	/* STEP 6: IF TWIST STATISTICS WERE REQUESTED, GO
	   THROUGH THE PATCHMAP AND CALCULATE TWIST & OMEGA */


	if (choice->boundary[0]) {

	    /* dynamically allocate storage for the arrays that will
	       hold the twist tallies and P values */

	    trows = patch->s - patch->n + 3;
	    tcols = patch->e - patch->w + 3;

	    twist2 = (int ***)G_calloc(trows + 3, sizeof(int **));
	    for (i = 0; i < trows + 3; i++) {
		twist2[i] = (int **)G_calloc(tcols + 3, sizeof(int *));
		for (j = 0; j < tcols + 3; j++)
		    twist2[i][j] = (int *)G_calloc(7, sizeof(int));
	    }

	    twistP = (float ***)G_calloc(trows + 3, sizeof(float **));
	    for (i = 0; i < trows + 3; i++) {
		twistP[i] = (float **)G_calloc(tcols + 3, sizeof(float *));
		for (j = 0; j < tcols + 3; j++)
		    twistP[i][j] = (float *)G_calloc(7, sizeof(float));
	    }

	    /* zero the twist2 and twistP matrices */

	    for (i = 0; i < trows + 3; i++) {
		for (j = 0; j < tcols + 3; j++) {
		    for (k = 0; k < 5; k++) {
			twist2[i][j][k] = 0;
			twistP[i][j][k] = 0.0;
		    }
		}
	    }

	    /* fill the twist2 matrix with counts; do this for
	       each pixel in the patch, identified by having a
	       value of 1 or -999 */

	    for (i = patch->n; i < patch->s + 1; i++) {
		for (j = patch->w; j < patch->e + 1; j++) {

		    n = i - patch->n + 1;
		    e = j - patch->w + 1;

		    if (*(*(patchmap + i) + j) > 0 ||
			*(*(patchmap + i) + j) == -999) {

			if (*(*(patchmap + i) + j - 1) > 0 ||
			    *(*(patchmap + i) + j - 1) == -999)
			    twist2[n][e][0]++;
			if (*(*(patchmap + i - 1) + j - 1) > 0 ||
			    *(*(patchmap + i - 1) + j - 1) == -999)
			    twist2[n][e][0]++;
			if (*(*(patchmap + i - 1) + j) > 0 ||
			    *(*(patchmap + i - 1) + j) == -999)
			    twist2[n][e][0]++;

			if (*(*(patchmap + i - 1) + j) > 0 ||
			    *(*(patchmap + i - 1) + j) == -999)
			    twist2[n][e][1]++;
			if (*(*(patchmap + i - 1) + j + 1) > 0 ||
			    *(*(patchmap + i - 1) + j + 1) == -999)
			    twist2[n][e][1]++;
			if (*(*(patchmap + i) + j + 1) > 0 ||
			    *(*(patchmap + i) + j + 1) == -999)
			    twist2[n][e][1]++;

			if (*(*(patchmap + i) + j + 1) > 0 ||
			    *(*(patchmap + i) + j + 1) == -999)
			    twist2[n][e][2]++;
			if (*(*(patchmap + i + 1) + j + 1) > 0 ||
			    *(*(patchmap + i + 1) + j + 1) == -999)
			    twist2[n][e][2]++;
			if (*(*(patchmap + i + 1) + j) > 0 ||
			    *(*(patchmap + i + 1) + j) == -999)
			    twist2[n][e][2]++;

			if (*(*(patchmap + i + 1) + j) > 0 ||
			    *(*(patchmap + i + 1) + j) == -999)
			    twist2[n][e][3]++;
			if (*(*(patchmap + i + 1) + j - 1) > 0 ||
			    *(*(patchmap + i + 1) + j - 1) == -999)
			    twist2[n][e][3]++;
			if (*(*(patchmap + i) + j - 1) > 0 ||
			    *(*(patchmap + i) + j - 1) == -999)
			    twist2[n][e][3]++;


			/* calculate the P values based on the tallies */

			for (k = 0; k < 4; k++) {
			    if (*(*(patchmap + i) + j) > 0 ||
				*(*(patchmap + i) + j) == -999) {

				if (twist2[n][e][k] - 1 < 0)
				    twistP[n][e][k] = 1.0;
				else if (twist2[n][e][k] - 1 == 0) {
				    if (k - 1 > 0)
					a = i + 1;
				    else
					a = i - 1;
				    if (k == 1 || k == 2)
					b = j + 1;
				    else
					b = j - 1;
				    if (*(*(patchmap + a) + b) > 0 ||
					*(*(patchmap + a) + b) == -999)
					twistP[n][e][k] = 1.0;
				    else
					twistP[n][e][k] = 0.0;
				}
				else if (twist2[n][e][k] - 1 > 0) {
				    if (twist2[n][e][k] == 3)
					twistP[n][e][k] = 0.0;
				    else if (twist2[n][e][k] == 2)
					twistP[n][e][k] = .33333;
				}
			    }
			}
		    }
		}
	    }

	    /* sum up the P values to calculate the twist number */

	    sumT = 0.0;
	    for (n = 0; n < trows; n++) {
		for (e = 0; e < tcols; e++) {
		    for (k = 0; k < 4; k++)
			sumT = sumT + twistP[n][e][k];
		}
	    }
	    patch->twist = (int)(sumT + 0.5);


	    /* calculate the omega index for 3 cases, depending upon
	       whether 8-neighbor or 4-neighbor tracing was chosen */

	    if (choice->trace) {
		if (patch->area > 1.0)
		    patch->omega = (4.0 * patch->area - (float)patch->twist) /
			(4.0 * patch->area - 4.0);
		else
		    patch->omega = 0.0;
	    }
	    else {
		if ((((int)patch->area % 4) - 1) == 0) {
		    if (patch->area > 1.0)
			patch->omega =
			    (2.0 * patch->area + 2.0 -
			     (float)patch->twist) / (2.0 * patch->area - 2.0);
		    else
			patch->omega = 0.0;
		}
		else
		    patch->omega = (2.0 * patch->area - (float)patch->twist) /
			(2.0 * patch->area - 4.0);
	    }

	    /*printf("twistnum = %4d omega=%6.4f area=%7.0f\n", patch->twist,
	       patch->omega,patch->area); */


	    /* free memory allocated for holding twist tallies and
	       P values */

	    for (i = 0; i < trows + 3; i++) {
		for (j = 0; j < tcols + 3; j++)
		    G_free(twistP[i][j]);
		G_free(twistP[i]);
	    }
	    G_free(twistP);

	    for (i = 0; i < trows + 3; i++) {
		for (j = 0; j < tcols + 3; j++)
		    G_free(twist2[i][j]);
		G_free(twist2[i]);
	    }
	    G_free(twist2);
	}



	/* STEP 7: MAKE NEXT PATCH NULL, FREE MEMORY,
	   AND RETURN THE PATCH STRUCTURE */

	patch->next = (PATCH *) NULL;

	/* free the memory allocated for patchmap */

	for (i = 0; i < nrows + 3; i++)
	    G_free(patchmap[i]);
	G_free(patchmap);

	/* send the patch info back to trace */

	return (patch);
    }				/*1 */
}







				/* SEARCH THE 8 NEIGHBORS OF A PIXEL IN
				   THE BUFFER IN A CLOCKWISE DIRECTION
				   LOOKING FOR A PIXEL WITH THE SAME
				   CLASS AND RETURN A 1 AND DI, DJ FOR
				   THE FIRST PIXEL FOUND; OTHERWISE RETURN
				   A ZERO */

int yes_nb(int *di, int *dj, DCELL ** buf, double class, int i, int j,
	   int nrows, int ncols)
{


    /*  di=0 to start; di is the value to be added to i to get to the
       pixel with the same value
       dj=-1 to start; dj is the value to be added to j to get to the
       pixel with the same value  
       class = the attribute of the center pixel
     */

    register int k;

    /*printf("1i=%d di=%d j=%d dj=%d nrows=%d 
       ncols=%d\n",i,*di,j,*dj,nrows,ncols); */

    /* if tracing is to include crossing to
       diagonal pixels */

    if (choice->trace) {	/*1 */

	/* search through the 8 neighbor pixels */

	for (k = 0; k < 8; k++) {

	    /* if the neighbor pixel has the same 
	       attribute as that of the current pixel,
	       then it may be part of the patch.
	       Confine the search to only pixels
	       inside the buffer to avoid a crash! */


	    if ((i + *di > 0) && (j + *dj > 0) &&
		(i + *di <= nrows) && (j + *dj <= ncols)) {

		if (class == *(*(buf + i + *di) + j + *dj))
		    return 1;

	    }
	    clockwise(di, dj);
	}

	/* if no neighbor with the same class is found,
	   then we are done tracing the patch */

	return 0;
    }				/*1 */

    /* if tracing is not to include crossing to
       diagonal pixels */

    else {

	/* search through the 8 neighbor pixels */

	for (k = 0; k < 8; k++) {

	    /* if the neighbor pixel has the same 
	       attribute as that of the current pixel,
	       then maybe it is part of the same patch */

	    if ((i + *di > 0) && (j + *dj > 0) &&
		(i + *di <= nrows) && (j + *dj <= ncols)) {

		if (class == *(*(buf + i + *di) + j + *dj)) {

		    /* if the neighbor pixel is directly above,
		       below, to the right or left of the current
		       pixel then tracing can continue */

		    if (*di == 0 || *dj == 0)
			return 1;

		    /* next check the diagonal neighbors 
		       that have a bishops pattern and if
		       they have an adjacent pixel with the same
		       class then continue tracing, as they are
		       not isolated diagonal pixels */

		    /* lower left bishops pattern */

		    if (*di == 1 && *dj == -1)
			if ((class == *(*(buf + i + *di) + j)) ||
			    (class == *(*(buf + i) + j + *dj)))
			    return 1;

		    /* upper left bishops pattern */

		    if (*di == -1 && *dj == -1)
			if ((class == *(*(buf + i + *di) + j)) ||
			    (class == *(*(buf + i) + j + *dj)))
			    return 1;

		    /* upper right bishops pattern */

		    if (*di == -1 && *dj == 1)
			if ((class == *(*(buf + i + *di) + j)) ||
			    (class == *(*(buf + i) + j + *dj)))
			    return 1;

		    /* lower right bishops pattern */

		    if (*di == 1 && *dj == 1)
			if ((class == *(*(buf + i + *di) + j)) ||
			    (class == *(*(buf + i) + j + *dj)))
			    return 1;
		}

	    }

	    /* if the neighbor pixel has a different
	       class or it is not in the
	       same row or col and is an isolated
	       bishops pattern pixel, then don't
	       trace it, but go to the next one
	       of the 8 neighbors */

	    clockwise(di, dj);

	}

	/* if all the neighbors are isolated
	   bishops pattern pixels or no 
	   neighbor with the same class is found
	   then we are done tracing the patch */

	return (0);
    }
}






				/* CIRCLE CLOCKWISE AROUND THE CURRENT PT */

void clockwise(int *i, int *j)
{
    if (*i != 0 && *j != -*i)
	*j -= *i;
    else
	*i += *j;
    return;
}
