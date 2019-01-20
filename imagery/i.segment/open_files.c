/* PURPOSE:      opening input rasters and creating segmentation files */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include <grass/segment.h>	/* segmentation library */
#include "iseg.h"

static int load_seeds(struct globals *, int, int, int);
static int read_seed(struct globals *, SEGMENT *, struct rc *, int);
static int manage_memory(int, int, struct globals *);

int open_files(struct globals *globals)
{
    int *in_fd, bounds_fd, is_null;
    int n, row, col, srows, scols, inlen, outlen, nseg;
    DCELL **inbuf;		/* buffers to store lines from each of the imagery group rasters */
    CELL *boundsbuf, bounds_val;
    int have_bounds = 0;
    CELL id;
    struct Range range;	/* min/max values of bounds map */
    struct FPRange *fp_range;	/* min/max values of each input raster */
    DCELL *min, *max;
    struct ngbr_stats Ri, Rk;

    /*allocate memory for flags */
    globals->null_flag = flag_create(globals->nrows, globals->ncols);
    globals->candidate_flag = flag_create(globals->nrows, globals->ncols);

    flag_clear_all(globals->null_flag);
    flag_clear_all(globals->candidate_flag);

    in_fd = G_malloc(globals->Ref.nfiles * sizeof(int));
    inbuf = (DCELL **) G_malloc(globals->Ref.nfiles * sizeof(DCELL *));
    fp_range = G_malloc(globals->Ref.nfiles * sizeof(struct FPRange));
    min = G_malloc(globals->Ref.nfiles * sizeof(DCELL));
    max = G_malloc(globals->Ref.nfiles * sizeof(DCELL));
    
    globals->min = min;
    globals->max = max;

    G_debug(1, "Opening input rasters...");
    for (n = 0; n < globals->Ref.nfiles; n++) {
	inbuf[n] = Rast_allocate_d_buf();
	in_fd[n] = Rast_open_old(globals->Ref.file[n].name, globals->Ref.file[n].mapset);
    }

    /* Get min/max values of each input raster for scaling */

    globals->max_diff = 0.;
    globals->nbands = globals->Ref.nfiles;

    for (n = 0; n < globals->Ref.nfiles; n++) {
	/* returns -1 on error, 2 on empty range, quitting either way. */
	if (Rast_read_fp_range(globals->Ref.file[n].name, globals->Ref.file[n].mapset, &fp_range[n]) != 1)
	    G_fatal_error(_("No min/max found in raster map <%s>"),
			  globals->Ref.file[n].name);
	Rast_get_fp_range_min_max(&(fp_range[n]), &min[n], &max[n]);
	if (Rast_is_d_null_value(&min[n])) {
	    G_fatal_error(_("Input map <%s> is all NULL"),
			  globals->Ref.file[n].name);
	}
	if (min[n] == max[n]) {
	    G_fatal_error(_("Input map <%s> is a constant of value %g"),
			  globals->Ref.file[n].name, min[n]);
	}

	G_debug(1, "Range for layer %d: min = %f, max = %f",
		    n, min[n], max[n]);
	
    }
    if (globals->weighted == FALSE)
	globals->max_diff = globals->Ref.nfiles;
    else {
	/* max difference with selected similarity method */
	Ri.mean = max;
	Rk.mean = min;
	globals->max_diff = 1;
	globals->max_diff = (*globals->calculate_similarity) (&Ri, &Rk, globals);
    }

    /* ********** find out file segmentation size ************ */
    G_debug(1, "Calculate temp file sizes...");

    /* size of each element to be stored */

    inlen = sizeof(DCELL) * globals->Ref.nfiles;
    outlen = sizeof(CELL);
    G_debug(1, "data element size, in: %d , out: %d ", inlen, outlen);
    globals->datasize = sizeof(double) * globals->nbands;

    /* count non-null cells */
    globals->notnullcells = (LARGEINT)globals->nrows * globals->ncols;
    for (row = 0; row < globals->nrows; row++) {
	for (n = 0; n < globals->Ref.nfiles; n++) {
	    Rast_get_d_row(in_fd[n], inbuf[n], row);
	}
	for (col = 0; col < globals->ncols; col++) {

	    is_null = 0;	/*Assume there is data */
	    for (n = 0; n < globals->Ref.nfiles; n++) {
		if (Rast_is_d_null_value(&inbuf[n][col])) {
		    is_null = 1;
		}
	    }
	    if (is_null) {
		globals->notnullcells--;
		FLAG_SET(globals->null_flag, row, col);
	    }
	}
    }
    if (globals->notnullcells < 2)
	G_fatal_error(_("Insufficient number of non-NULL cells in current region"));

    /* segment lib segment size */
    srows = 64;
    scols = 64;

    nseg = manage_memory(srows, scols, globals);

    /* create segment structures */
    if (Segment_open
	(&globals->bands_seg, G_tempfile(), globals->nrows, globals->ncols, srows,
	 scols, inlen, nseg) != 1)
	G_fatal_error("Unable to create input temporary files");

    if (globals->method == ORM_MS) {
	if (Segment_open
	    (&globals->bands_seg2, G_tempfile(), globals->nrows, globals->ncols, srows,
	     scols, inlen, nseg) != 1)
	    G_fatal_error("Unable to create input temporary files");
	
	globals->bands_in = &globals->bands_seg;
	globals->bands_out = &globals->bands_seg2;
    }

    if (Segment_open
	(&globals->rid_seg, G_tempfile(), globals->nrows, globals->ncols, srows,
	 scols, outlen, nseg * 2) != 1)
	G_fatal_error("Unable to create input temporary files");

    /* load input bands to segment structure */
    if (globals->Ref.nfiles > 1)
	G_message(_("Loading input bands..."));
    else
	G_message(_("Loading input band..."));

    globals->bands_val = (double *)G_malloc(inlen);
    globals->second_val = (double *)G_malloc(inlen);

    globals->max_rid = 0;

    globals->row_min = globals->nrows;
    globals->row_max = 0;
    globals->col_min = globals->ncols;
    globals->col_max = 0;
    for (row = 0; row < globals->nrows; row++) {
	G_percent(row, globals->nrows, 4);
	for (n = 0; n < globals->Ref.nfiles; n++) {
	    Rast_get_d_row(in_fd[n], inbuf[n], row);
	}
	for (col = 0; col < globals->ncols; col++) {

	    is_null = 0;	/*Assume there is data */
	    for (n = 0; n < globals->Ref.nfiles; n++) {
		globals->bands_val[n] = inbuf[n][col];
		if (Rast_is_d_null_value(&inbuf[n][col])) {
		    is_null = 1;
		}
		else {
		    if (globals->weighted == FALSE)
		    	/* scaled version */
			globals->bands_val[n] = (inbuf[n][col] - min[n]) / (max[n] - min[n]);
		}
	    }
	    if (Segment_put(&globals->bands_seg,
	                    (void *)globals->bands_val, row, col) != 1)
		G_fatal_error(_("Unable to write to temporary file"));

	    if (globals->method == ORM_MS) {
		if (Segment_put(&globals->bands_seg2,
				(void *)globals->bands_val, row, col) != 1)
		    G_fatal_error(_("Unable to write to temporary file"));
	    }

	    id = 0;
	    if (!is_null) {
		/* get min/max row/col to narrow the processing window */
		if (globals->row_min > row)
		    globals->row_min = row;
		if (globals->row_max < row)
		    globals->row_max = row;
		if (globals->col_min > col)
		    globals->col_min = col;
		if (globals->col_max < col)
		    globals->col_max = col;
	    }
	    else {
		/* all input bands NULL */
		Rast_set_c_null_value(&id, 1);
		FLAG_SET(globals->null_flag, row, col);
	    }
	    if (Segment_put(&globals->rid_seg,
			    (void *)&id, row, col) != 1)
		G_fatal_error(_("Unable to write to temporary file"));
	}
    }
    G_percent(1, 1, 1);
    G_debug(1, "nrows: %d, min row: %d, max row %d",
	       globals->nrows, globals->row_min, globals->row_max);
    G_debug(1, "ncols: %d, min col: %d, max col %d",
               globals->ncols, globals->col_min, globals->col_max);
    
    globals->row_max++;
    globals->col_max++;
    globals->ncells = (LARGEINT)(globals->row_max - globals->row_min) *
			    (globals->col_max - globals->col_min);

    /* bounds/constraints */

    Rast_set_c_null_value(&globals->upper_bound, 1);
    Rast_set_c_null_value(&globals->lower_bound, 1);

    if (globals->bounds_map != NULL) {
	if (Segment_open
	    (&globals->bounds_seg, G_tempfile(), globals->nrows, globals->ncols,
	     srows, scols, sizeof(CELL), nseg) != TRUE)
	    G_fatal_error("Unable to create bounds temporary files");

	if (Rast_read_range(globals->bounds_map, globals->bounds_mapset, &range) != 1)
	    G_fatal_error(_("No min/max found in raster map <%s>"),
			  globals->bounds_map);
	Rast_get_range_min_max(&range, &globals->upper_bound,
				       &globals->lower_bound);

	if (Rast_is_c_null_value(&globals->upper_bound) ||
	    Rast_is_c_null_value(&globals->lower_bound)) {
	    
	    G_fatal_error(_("No min/max found in raster map <%s>"),
	                  globals->bounds_map);
	}

	bounds_fd = Rast_open_old(globals->bounds_map, globals->bounds_mapset);
	boundsbuf = Rast_allocate_c_buf();

	for (row = 0; row < globals->nrows; row++) {
	    Rast_get_c_row(bounds_fd, boundsbuf, row);
	    for (col = 0; col < globals->ncols; col++) {
		bounds_val = boundsbuf[col];
		if (FLAG_GET(globals->null_flag, row, col)) {
		    Rast_set_c_null_value(&bounds_val, 1);
		}
		else {
		    if (!Rast_is_c_null_value(&bounds_val)) {
			have_bounds = 1;
			if (globals->lower_bound > bounds_val)
			    globals->lower_bound = bounds_val;
			if (globals->upper_bound < bounds_val)
			    globals->upper_bound = bounds_val;
		    }
		}
		if (Segment_put(&globals->bounds_seg, &bounds_val, row, col) != 1)
		    G_fatal_error(_("Unable to write to temporary file"));
	    }
	}
	Rast_close(bounds_fd);
	G_free(boundsbuf);

	if (!have_bounds) {
	    G_warning(_("There are no boundary constraints in '%s'"), globals->bounds_map);
	    Rast_set_c_null_value(&globals->upper_bound, 1);
	    Rast_set_c_null_value(&globals->lower_bound, 1);
	    Segment_close(&globals->bounds_seg);
	    globals->bounds_map = NULL;
	    globals->bounds_mapset = NULL;
	}
    }
    else {
	G_debug(1, "no boundary constraint supplied.");
    }

    /* other info */
    globals->candidate_count = 0;	/* counter for remaining candidate pixels */

    /* Free memory */

    for (n = 0; n < globals->Ref.nfiles; n++) {
	G_free(inbuf[n]);
	Rast_close(in_fd[n]);
    }

    globals->rs.sum = G_malloc(globals->datasize);
    globals->rs.mean = G_malloc(globals->datasize);

    globals->reg_tree = rgtree_create(globals->nbands, globals->datasize);

    if (globals->method == ORM_RG && globals->seeds) {
	load_seeds(globals, srows, scols, nseg);
	G_debug(1, "Number of initial regions: %d", globals->max_rid);
    }

    G_free(inbuf);
    G_free(in_fd);
    G_free(fp_range);

    return TRUE;
}


static int load_seeds(struct globals *globals, int srows, int scols, int nseg)
{
    int row, col;
    SEGMENT seeds_seg;
    CELL *seeds_buf, seeds_val;
    int seeds_fd, have_seeds;
    CELL new_id, cellmax, noid;
    struct rc Ri;

    G_debug(1, "load_seeds()");

    cellmax = (1 << (sizeof(CELL) * 8 - 2)) - 1;
    cellmax += (1 << (sizeof(CELL) * 8 - 2));

    noid = 0;

    G_message(_("Loading seeds from raster map <%s>..."), globals->seeds);

    if (Segment_open
	(&seeds_seg, G_tempfile(), globals->nrows, globals->ncols,
	 srows, scols, sizeof(CELL), nseg) != TRUE)
	G_fatal_error("Unable to create bounds temporary files");

    seeds_fd = Rast_open_old(globals->seeds, "");
    seeds_buf = Rast_allocate_c_buf();
    
    have_seeds = 0;

    /* load seeds map to segment structure */
    for (row = 0; row < globals->nrows; row++) {
	Rast_get_c_row(seeds_fd, seeds_buf, row);
	for (col = 0; col < globals->ncols; col++) {
	    if (FLAG_GET(globals->null_flag, row, col)) {
		Rast_set_c_null_value(&seeds_val, 1);
	    }
	    else {
		seeds_val = seeds_buf[col];
		if (!Rast_is_c_null_value(&seeds_val))
		    have_seeds = 1;
	    }
	    if (Segment_put(&seeds_seg, &seeds_val, row, col) != 1)
		G_fatal_error(_("Unable to write to temporary file"));
	}
    }

    if (!have_seeds) {
	G_warning(_("No seeds found in '%s'!"), globals->seeds);
	G_free(seeds_buf);
	Rast_close(seeds_fd);
	Segment_close(&seeds_seg);
	return 0;
    }

    new_id = 0;

    /* convert seeds to regions */
    G_debug(1, "convert seeds to regions");
    Rast_set_c_null_value(&seeds_val, 1);
    for (row = 0; row < globals->nrows; row++) {
	Rast_get_c_row(seeds_fd, seeds_buf, row);
	for (col = 0; col < globals->ncols; col++) {
	    if (!(FLAG_GET(globals->null_flag, row, col)) && 
	        !(FLAG_GET(globals->candidate_flag, row, col))) {

		if (!Rast_is_c_null_value(&(seeds_buf[col]))) {
		    if (new_id == cellmax)
			G_fatal_error(_("Too many seeds: integer overflow"));

		    new_id++;

		    Ri.row = row;
		    Ri.col = col;
		    if (!read_seed(globals, &seeds_seg, &Ri, new_id)) {
			new_id--;
			Segment_put(&globals->rid_seg, (void *)&noid, Ri.row, Ri.col);
		    }
		}
	    }
	}
    }

    G_free(seeds_buf);
    Rast_close(seeds_fd);
    Segment_close(&seeds_seg);

    globals->max_rid = new_id;
    
    flag_clear_all(globals->candidate_flag);
    
    return 1;
}


static int read_seed(struct globals *globals, SEGMENT *seeds_seg, struct rc *Ri, int new_id)
{
    int n, i, Ri_id, Rk_id;
    struct rc ngbr_rc, next;
    struct rclist rilist;
    int neighbors[8][2];

    G_debug(4, "read_seed()");

    /* get Ri's segment ID from input seeds */
    Segment_get(seeds_seg, &Ri_id, Ri->row, Ri->col);
    
    /* set new segment id */
    if (Segment_put(&globals->rid_seg, &new_id, Ri->row, Ri->col) != 1)
	G_fatal_error(_("Unable to write to temporary file"));
    /* set candidate flag */
    FLAG_SET(globals->candidate_flag, Ri->row, Ri->col);

    /* initialize region stats */
    globals->rs.count = 1;

    globals->rs.id = new_id;
    Segment_get(&globals->bands_seg, (void *)globals->bands_val,
		Ri->row, Ri->col);

    for (i = 0; i < globals->nbands; i++) {
	globals->rs.sum[i] = globals->bands_val[i];
	globals->rs.mean[i] = globals->bands_val[i];
    }

    /* go through seed, spreading outwards from head */
    rclist_init(&rilist);
    rclist_add(&rilist, Ri->row, Ri->col);

    while (rclist_drop(&rilist, &next)) {

	G_debug(5, "find_pixel_neighbors for row: %d , col %d",
		next.row, next.col);

	globals->find_neighbors(next.row, next.col, neighbors);
	
	for (n = 0; n < globals->nn; n++) {

	    ngbr_rc.row = neighbors[n][0];
	    ngbr_rc.col = neighbors[n][1];

	    if (ngbr_rc.row < 0 || ngbr_rc.row >= globals->nrows ||
		ngbr_rc.col < 0 || ngbr_rc.col >= globals->ncols) {
		continue;
	    }

	    if (FLAG_GET(globals->null_flag, ngbr_rc.row, ngbr_rc.col)) {
		continue;
	    }

	    if (FLAG_GET(globals->candidate_flag, ngbr_rc.row, ngbr_rc.col)) {
		continue;
	    }

	    Segment_get(seeds_seg, (void *) &Rk_id, ngbr_rc.row, ngbr_rc.col);
		
	    G_debug(5, "Rk ID = %d Ri ID = %d", Rk_id, Ri_id);

	    if (Rk_id != Ri_id) {
		continue;
	    }

	    /* set segment id */
	    if (Segment_put(&globals->rid_seg,
	                    &new_id, ngbr_rc.row, ngbr_rc.col) != 1)
		G_fatal_error(_("Unable to write to temporary file"));
	    
	    /* set candidate flag */
	    FLAG_SET(globals->candidate_flag, ngbr_rc.row, ngbr_rc.col);
    
	    /* add to list of cells to check */
	    rclist_add(&rilist, ngbr_rc.row, ngbr_rc.col);
	    
	    /* update region stats */
	    Segment_get(&globals->bands_seg, (void *)globals->bands_val,
			ngbr_rc.row, ngbr_rc.col);

	    for (i = 0; i < globals->nbands; i++) {
		globals->rs.sum[i] += globals->bands_val[i];
	    }
	    globals->rs.count++;
	}
    }

    if (rgtree_find(globals->reg_tree, &(globals->rs)) != NULL) {
	G_fatal_error(_("Segment %d is already registered!"), new_id);
    }
    
    /* insert into region tree */
    if (globals->rs.count >= globals->min_reg_size) {
	for (i = 0; i < globals->nbands; i++)
	    globals->rs.mean[i] = globals->rs.sum[i] / globals->rs.count;

	rgtree_insert(globals->reg_tree, &(globals->rs));
    }
    else {
	if (globals->rs.count > 1)
	    update_band_vals(Ri->row, Ri->col, &(globals->rs), globals);
	else if (globals->rs.count == 1) {
	    return 0;
	}
    }

    return 1;
}


static int manage_memory(int srows, int scols, struct globals *globals)
{
    double reg_size_mb, segs_mb;
    LARGEINT reg_size_count;
    int nseg, nseg_total;
    
    segs_mb = globals->mb;
    if (globals->method == ORM_RG) {

	/* minimum region size to store in search tree */
	reg_size_mb = 2 * globals->datasize +     /* mean, sum */
		      2 * sizeof(int) +           /* id, count */
		      sizeof(unsigned char) + 
		      2 * sizeof(struct REG_NODE *);
	reg_size_mb /= (1024. * 1024.);

	/* put aside some memory for segment structures */
	segs_mb = globals->mb * 0.1;
	if (segs_mb > 10)
	    segs_mb = 10;

	/* calculate number of region stats that can be kept in memory */
	reg_size_count = (globals->mb - segs_mb) / reg_size_mb;
	if (reg_size_count < 1)
	    reg_size_count = 1;
	globals->min_reg_size = 3;
	if (reg_size_count < (double) globals->notnullcells / globals->min_reg_size) {
	    globals->min_reg_size = (double) globals->notnullcells / reg_size_count;
	}
	else {
	    reg_size_count = (double) globals->notnullcells / globals->min_reg_size;
	    /* recalculate segs_mb */
	    segs_mb = globals->mb - reg_size_count * reg_size_mb;
	    if (segs_mb < 10)
		segs_mb = 10;
	}

	G_verbose_message(_("Regions with at least %"PRI_LONG" cells are stored in memory"),
			  globals->min_reg_size);
    }

    /* calculate number of segments in memory */
    /* nseg: integer overflow possible with large segs_mb */
    if (globals->bounds_map != NULL) {
	/* input bands, segment ids, bounds map */
	if (globals->method == ORM_MS) {
	    nseg = (1024. * 1024. * segs_mb) /
		   (sizeof(DCELL) * 2 * globals->nbands * srows * scols + 
		    sizeof(CELL) * 4 * srows * scols);
	}
	else {
	    nseg = (1024. * 1024. * segs_mb) /
		   (sizeof(DCELL) * globals->nbands * srows * scols + 
		    sizeof(CELL) * 4 * srows * scols);
	}
    }
    else {
	/* input bands, segment ids */
	if (globals->method == ORM_MS) {
	    nseg = (1024. * 1024. * segs_mb) /
		   (sizeof(DCELL) * 2 * globals->nbands * srows * scols + 
		    sizeof(CELL) * 2 * srows * scols);
	}
	else {
	    nseg = (1024. * 1024. * segs_mb) /
		   (sizeof(DCELL) * globals->nbands * srows * scols + 
		    sizeof(CELL) * 2 * srows * scols);
	}
    }
    nseg_total = (globals->nrows / srows + (globals->nrows % srows > 0)) *
                 (globals->ncols / scols + (globals->ncols % scols > 0));

    if (nseg > nseg_total)
	nseg = nseg_total;
    
    G_debug(1, "current region:  %d rows, %d cols", globals->nrows, globals->ncols);
    G_debug(1, "segmented to tiles with size:  %d rows, %d cols", srows,
	    scols);
    G_verbose_message(_("Number of segments in memory: %d of %d total"),
                      nseg, nseg_total);
    
    return nseg;
}
