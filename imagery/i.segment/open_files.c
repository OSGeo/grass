/* PURPOSE:      opening input rasters and creating segmentation files */

#include <limits.h>		/* for INT_MAX */
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include <grass/segment.h>	/* segmentation library */
#include "iseg.h"

int open_files(struct files *files, struct functions *functions)
{
    struct Ref Ref;		/* group reference list */
    int *in_fd, seeds_fd, bounds_fd, null_check, out_fd, mean_fd;
    int n, s, row, col, srows, scols, inlen, nseg, borderPixels;
    DCELL **inbuf;		/* buffer array, to store lines from each of the imagery group rasters */
    CELL *boundsbuf, *seedsbuf;
    void *ptr;			/* for iterating seedsbuf */
    size_t ptrsize;
    struct FPRange *fp_range;	/* for getting min/max values on each input raster */
    DCELL *min, *max;
    struct Range range;		/* for seeds range */
    int seeds_min, seeds_max;


    /* for merging seed values */
    struct pixels *R_head, *Rn_head, *newpixel, *current;
    int R_count;

    G_verbose_message(_("Opening files and initializing"));

    /* confirm output maps can be opened (don't want to do all this work for nothing!) */
    out_fd = Rast_open_new(files->out_name, CELL_TYPE);
    if (out_fd < 0)
	G_fatal_error(_("Could not open output raster for writing segment ID's"));
    else
	Rast_unopen(out_fd);

    if (files->out_band != NULL) {
	mean_fd = Rast_open_new(files->out_band, DCELL_TYPE);
	if (mean_fd < 0)
	    G_fatal_error(_("Could not open output raster for writing mean segment values"));
	else
	    Rast_unopen(mean_fd);
    }

    /*allocate memory for flags */
    files->null_flag = flag_create(files->nrows, files->ncols);
    flag_clear_all(files->null_flag);
    files->candidate_flag = flag_create(files->nrows, files->ncols);

    if (files->bounds_map != NULL)
	files->orig_null_flag = flag_create(files->nrows, files->ncols);

    files->seeds_flag = flag_create(files->nrows, files->ncols);
    flag_clear_all(files->seeds_flag);

    /* references for segmentation library: i.cost r.watershed/seg and http://grass.osgeo.org/programming7/segmentlib.html */

    /* ****** open the input rasters ******* */

    /* Note: I confirmed, the API does not check this: */
    if (!I_get_group_ref(files->image_group, &Ref))
	G_fatal_error(_("Unable to read REF file for group <%s>"),
		      files->image_group);

    if (Ref.nfiles <= 0)
	G_fatal_error(_("Group <%s> contains no raster maps"),
		      files->image_group);

    /* Read Imagery Group */

    in_fd = G_malloc(Ref.nfiles * sizeof(int));
    inbuf = (DCELL **) G_malloc(Ref.nfiles * sizeof(DCELL *));
    fp_range = G_malloc(Ref.nfiles * sizeof(struct FPRange));
    min = G_malloc(Ref.nfiles * sizeof(DCELL));
    max = G_malloc(Ref.nfiles * sizeof(DCELL));

    for (n = 0; n < Ref.nfiles; n++) {
	inbuf[n] = Rast_allocate_d_buf();
	in_fd[n] = Rast_open_old(Ref.file[n].name, Ref.file[n].mapset);
	if (in_fd[n] < 0)
	    G_fatal_error(_("Error opening %s@%s"), Ref.file[n].name,
			  Ref.file[n].mapset);
    }

    /* open seeds raster and confirm all positive integers were given */
    if (files->seeds_map != NULL) {
	seeds_fd = Rast_open_old(files->seeds_map, "");
	seedsbuf = Rast_allocate_c_buf();
	ptrsize = sizeof(CELL);

	if (Rast_read_range(files->seeds_map, files->seeds_mapset, &range) != 1) {	/* returns -1 on error, 2 on empty range, quiting either way. */
	    G_fatal_error(_("No min/max found in seeds raster map <%s>"),
			  files->seeds_map);
	}
	Rast_get_range_min_max(&range, &seeds_min, &seeds_max);
	if (seeds_min < 0)
	    G_fatal_error(_("Seeds raster should have postive integers for starting seeds, and zero or NULL for all other pixels."));
    }

    /* Get min/max values of each input raster for scaling */

    if (files->weighted == FALSE) {	/*default, we will scale */
	for (n = 0; n < Ref.nfiles; n++) {
	    if (Rast_read_fp_range(Ref.file[n].name, Ref.file[n].mapset, &fp_range[n]) != 1)	/* returns -1 on error, 2 on empty range, quiting either way. */
		G_fatal_error(_("No min/max found in raster map <%s>"),
			      Ref.file[n].name);
	    Rast_get_fp_range_min_max(&(fp_range[n]), &min[n], &max[n]);
	}
    }

    /* ********** find out file segmentation size ************ */

    files->nbands = Ref.nfiles;

    /* size of each element to be stored */

    /* save for bands, plus area, perimeter, bounding box. */

    inlen = sizeof(double) * (Ref.nfiles + 6);

    /* when fine tuning, should be a power of 2 and not larger than 256 for speed reasons */
    srows = 64;
    scols = 64;

    /* RAM enhancement: have user input limit and make calculations for this, reference i.cost and i.watershed 
     * One segment tile is tile_mb = (nbands * sizeof(double) + sizeof(CELL) * srows * scols / (1024*1024) 
     * (check if sizeof(CELL) was from when iseg would be included, or the extra overhead?)
     * If user inputs total RAM available, need to subtract the size of the flags, and the size of the linked lists.
     * I'm not sure how to estimate the size of the linked lists, it is allowed to grow when needed.  Assume one segment
     * could be 50% of the map?  Or more?  So ll_mb = sizeof(pixels) * nrows * ncols * 0.5 / (1024*1024)
     * then split the remaining RAM between bands_seg and iseg_seg. */

    nseg = 16;


    /* ******* create temporary segmentation files ********* */
    /* Initalize access to database and create temporary files */

    G_debug(1, "Image size:  %d rows, %d cols", files->nrows, files->ncols);
    G_debug(1, "Segmented to tiles with size:  %d rows, %d cols", srows,
	    scols);
    G_debug(1, "Data element size, in: %d", inlen);
    G_debug(1, "number of segments to have in memory: %d", nseg);

    if (segment_open
	(&files->bands_seg, G_tempfile(), files->nrows, files->ncols, srows,
	 scols, inlen, nseg) != TRUE)
	G_fatal_error("Unable to create input temporary files");

    /* ******* remaining memory allocation ********* */

    /* save the area and perimeter as well */
    /* perimeter todo: currently saving this with the input DCELL values.  
     * Better to have a second segment structure to save as integers ??? */
    /* along with P and A, also saving the bounding box - min/max row/col */

    /* Why was this being reset here??? commented out... 
       inlen = inlen + sizeof(double) * 6; */

    files->bands_val = (double *)G_malloc(inlen);
    files->second_val = (double *)G_malloc(inlen);

    if (segment_open
	(&files->iseg_seg, G_tempfile(), files->nrows, files->ncols, srows,
	 scols, sizeof(int), nseg) != TRUE)
	G_fatal_error(_("Unable to allocate memory for initial segment ID's"));

    /* bounds/constraints (start with processing constraints to get any possible NULL values) */
    if (files->bounds_map != NULL) {
	if (segment_open
	    (&files->bounds_seg, G_tempfile(), files->nrows, files->ncols,
	     srows, scols, sizeof(int), nseg) != TRUE)
	    G_fatal_error(_("Unable to create bounds temporary files"));

	boundsbuf = Rast_allocate_c_buf();
	bounds_fd = Rast_open_old(files->bounds_map, files->bounds_mapset);

	for (row = 0; row < files->nrows; row++) {
	    Rast_get_c_row(bounds_fd, boundsbuf, row);
	    for (col = 0; col < files->ncols; col++) {
		files->bounds_val = boundsbuf[col];
		segment_put(&files->bounds_seg, &files->bounds_val, row, col);
		if (Rast_is_c_null_value(&boundsbuf[col]) == TRUE) {
		    FLAG_SET(files->null_flag, row, col);
		}
	    }
	}
	Rast_close(bounds_fd);
	G_free(boundsbuf);
    }				/* end bounds/constraints opening */


    /* ********  load input bands to segment structure and fill initial seg ID's ******** */
    s = 0;			/* initial segment ID will be 1 */

    for (row = 0; row < files->nrows; row++) {

	/* read in rows of data (each input band from the imagery group and the optional seeds map) */
	for (n = 0; n < Ref.nfiles; n++) {
	    Rast_get_d_row(in_fd[n], inbuf[n], row);
	}
	if (files->seeds_map != NULL) {
	    Rast_get_c_row(seeds_fd, seedsbuf, row);
	    ptr = seedsbuf;
	}

	for (col = 0; col < files->ncols; col++) {
	    if (FLAG_GET(files->null_flag, row, col))
		continue;
	    null_check = 1;	/*Assume there is data */
	    for (n = 0; n < Ref.nfiles; n++) {
		if (Rast_is_d_null_value(&inbuf[n][col]))
		    null_check = -1;
		if (files->weighted == TRUE)
		    files->bands_val[n] = inbuf[n][col];	/*unscaled */
		else
		    files->bands_val[n] = (inbuf[n][col] - min[n]) / (max[n] - min[n]);	/* scaled */
	    }

	    /* besides the user input rasters, also save the shape parameters */

	    files->bands_val[Ref.nfiles] = 1;	/* area (just using the number of pixels) */
	    files->bands_val[Ref.nfiles + 1] = 4;	/* Perimeter Length *//* todo perimeter, not exact for edges...close enough for now? */
	    files->bands_val[Ref.nfiles + 2] = col;	/*max col */
	    files->bands_val[Ref.nfiles + 3] = col;	/*min col */
	    files->bands_val[Ref.nfiles + 4] = row;	/*max row */
	    files->bands_val[Ref.nfiles + 5] = row;	/*min row */

	    segment_put(&files->bands_seg, (void *)files->bands_val, row, col);	/* store input bands */
	    if (null_check != -1) {	/*good pixel */
		FLAG_UNSET(files->null_flag, row, col);	/*flag */
		if (files->seeds_map != NULL) {
		    if (Rast_is_c_null_value(ptr) == TRUE) {
			/* when using iseg_seg the segmentation file is already initialized to zero.  Just initialize seeds_flag: */
			FLAG_UNSET(files->seeds_flag, row, col);
		    }
		    else {
			FLAG_SET(files->seeds_flag, row, col);	/* RAM enhancement, but it might cost speed.  Could look for seg ID > 0 instead of using seed_flag. */
			/* seed value is starting segment ID. */
			segment_put(&files->iseg_seg, ptr, row, col);
		    }
		    ptr = G_incr_void_ptr(ptr, ptrsize);
		}
		else {		/* no seeds provided */
		    s++;	/* sequentially number all pixels with their own segment ID */
		    if (s < INT_MAX) {	/* Check that the starting seeds aren't too large. */
			segment_put(&files->iseg_seg, &s, row, col);	/*starting segment number */
			FLAG_SET(files->seeds_flag, row, col);	/*all pixels are seeds */
		    }
		    else
			G_fatal_error(_("Exceeded integer storage limit, too many initial pixels."));
		}
	    }
	    else {		/*don't use this pixel */
		FLAG_SET(files->null_flag, row, col);	/*flag */
	    }
	}
    }

    /* keep original copy of null flag if we have boundary constraints */
    if (files->bounds_map != NULL) {
	for (row = 0; row < files->nrows; row++) {
	    for (col = 0; col < files->ncols; col++) {
		if (FLAG_GET(files->null_flag, row, col))
		    FLAG_SET(files->orig_null_flag, row, col);
		else
		    FLAG_UNSET(files->orig_null_flag, row, col);
	    }
	}
    }				/* end: if (files->bounds_map != NULL) */

    /* linked list memory management linkm */
    link_set_chunk_size(1000);	/* TODO RAM: fine tune this number */

    files->token = link_init(sizeof(struct pixels));

    /* if we have seeds that are segments (not pixels) we need to update the bands_seg */
    /* also renumber the segment ID's in case they were classified 
     * (duplicating numbers) instead of output from i.segment. */
    if (files->seeds_map != NULL) {

	/*initialization */
	files->minrow = files->mincol = 0;
	files->maxrow = files->nrows;
	files->maxcol = files->ncols;
	R_count = 1;
	R_head = NULL;
	Rn_head = NULL;
	newpixel = NULL;
	current = NULL;
	set_all_candidate_flags(files);
	for (row = 0; row < files->nrows; row++) {
	    G_percent(row, files->nrows, 1);	/* I think this is the longest part of open_files()
						 * - not entirely accurate for the actual %,
						 *  but will give the user something to see. */
	    for (col = 0; col < files->ncols; col++) {
		if (!(FLAG_GET(files->candidate_flag, row, col)) ||
		    FLAG_GET(files->null_flag, row, col))
		    continue;
		/*start R_head */
		newpixel = (struct pixels *)link_new(files->token);
		newpixel->next = NULL;
		newpixel->row = row;
		newpixel->col = col;
		R_head = newpixel;

		/* get pixel list, possible initialization speed enhancement: 
		 * could use a custom (shorter) function, some results from 
		 * find_segment_neighbors are not used here */
		/* bug todo: There is a small chance that a renumbered segment 
		 * matches and borders an original segment.  This would be a 
		 * good reason to write a custom function - use the candidate 
		 * flag to see if the pixel was already processed. */
		borderPixels =
		    find_segment_neighbors(&R_head, &Rn_head, &R_count, files,
					   functions);

		/* update the segment ID */

		s++;
		if (s == INT_MAX)	/* Check that the starting seeds aren't too large. */
		    G_fatal_error(_("Exceeded integer storage limit, too many initial pixels."));

		for (current = R_head; current != NULL;
		     current = current->next) {
		    segment_put(&files->iseg_seg, &s, current->row,
				current->col);
		    FLAG_UNSET(files->candidate_flag, current->row,
			       current->col);
		}

		/*merge pixels (updates the bands_seg) */
		merge_pixels(R_head, borderPixels, files);

		/*todo calculate perimeter (?and area?) here? */

		/*clean up */
		my_dispose_list(files->token, &R_head);
		my_dispose_list(files->token, &Rn_head);
		R_count = 1;
	    }
	}
    }

    files->nsegs = s;

    /* Free memory */

    for (n = 0; n < Ref.nfiles; n++) {
	G_free(inbuf[n]);
	Rast_close(in_fd[n]);
    }

    if (files->seeds_map != NULL) {
	Rast_close(seeds_fd);
	G_free(seedsbuf);
    }

    G_free(inbuf);
    G_free(in_fd);
    G_free(fp_range);
    G_free(min);
    G_free(max);

    return TRUE;
}
