#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/segment.h>	/* segmentation library */
#include <grass/glocale.h>
#include "iseg.h"

int write_ids(struct globals *globals)
{
    int out_fd, row, col, maxid;
    CELL *outbuf, rid;
    struct Colors colors;
    struct History hist;

    outbuf = Rast_allocate_c_buf();

    G_debug(1, "preparing output raster");
    /* open output raster map */
    out_fd = Rast_open_new(globals->out_name, CELL_TYPE);

    G_debug(1, "start data transfer from segmentation file to raster");

    G_message(_("Writing out segment IDs..."));
    maxid = 0;
    for (row = 0; row < globals->nrows; row++) {

	G_percent(row, globals->nrows, 9);

	Rast_set_c_null_value(outbuf, globals->ncols);
	for (col = 0; col < globals->ncols; col++) {

	    if (!(FLAG_GET(globals->null_flag, row, col))) {
		Segment_get(&globals->rid_seg, (void *) &rid, row, col);

		if (rid > 0) {
		    if (globals->method == ORM_RG)
			rid = globals->new_id[rid];
		    outbuf[col] = rid;
		    if (maxid < rid)
			maxid = rid;
		}
	    }
	}
	Rast_put_row(out_fd, outbuf, CELL_TYPE);
    }
    G_percent(1, 1, 1);
    
    /* close and save segment id file */
    Rast_close(out_fd);
    G_free(outbuf);

    /* set colors */
    Rast_init_colors(&colors);
    Rast_make_random_colors(&colors, 1, maxid);
    Rast_write_colors(globals->out_name, G_mapset(), &colors);

    Rast_short_history(globals->out_name, "raster", &hist);
    Rast_command_history(&hist);
    Rast_write_history(globals->out_name, &hist);

    /* free memory */
    Rast_free_colors(&colors);

    return TRUE;
}

/* write goodness of fit */
int write_gof_rg(struct globals *globals)
{
    int row, col;
    int mean_fd;
    CELL rid;
    FCELL *meanbuf;
    double thresh, maxdev, sim, mingood;
    struct ngbr_stats Ri, Rk;
    DCELL **inbuf;		/* buffers to store lines from each of the imagery group rasters */
    int n, *in_fd;
    struct FPRange *fp_range;	/* min/max values of each input raster */
    struct Colors colors;
    struct History hist;
    DCELL *min, *max;

    mean_fd = Rast_open_new(globals->gof, FCELL_TYPE);
    meanbuf = Rast_allocate_f_buf();

    /* goodness of fit for each cell: 1 = good fit, 0 = bad fit */
    /* similarity of each cell to region mean
     * max possible difference: globals->threshold
     * if similarity < globals->alpha * globals->alpha * globals->threshold
     * 1
     * else 
     * (similarity - globals->alpha * globals->alpha * globals->threshold) /
     * (globals->threshold * (1 - globals->alpha * globals->alpha) */

    thresh = globals->alpha * globals->alpha * globals->max_diff;
    maxdev = globals->max_diff * (1 - globals->alpha * globals->alpha);
    mingood = 1;

    /* open input bands */
    in_fd = G_malloc(globals->Ref.nfiles * sizeof(int));
    inbuf = (DCELL **) G_malloc(globals->Ref.nfiles * sizeof(DCELL *));
    fp_range = G_malloc(globals->Ref.nfiles * sizeof(struct FPRange));
    min = G_malloc(globals->Ref.nfiles * sizeof(DCELL));
    max = G_malloc(globals->Ref.nfiles * sizeof(DCELL));

    G_debug(1, "Opening input rasters...");
    for (n = 0; n < globals->Ref.nfiles; n++) {
	inbuf[n] = Rast_allocate_d_buf();
	in_fd[n] = Rast_open_old(globals->Ref.file[n].name, globals->Ref.file[n].mapset);

	/* returns -1 on error, 2 on empty range, quitting either way. */
	if (Rast_read_fp_range(globals->Ref.file[n].name, globals->Ref.file[n].mapset, &fp_range[n]) != 1)
	    G_fatal_error(_("No min/max found in raster map <%s>"),
			  globals->Ref.file[n].name);
	Rast_get_fp_range_min_max(&(fp_range[n]), &min[n], &max[n]);

	G_debug(1, "Range for layer %d: min = %f, max = %f",
		    n, min[n], max[n]);
    }

    G_message(_("Writing out goodness of fit"));
    for (row = 0; row < globals->nrows; row++) {

	G_percent(row, globals->nrows, 9);

	Rast_set_f_null_value(meanbuf, globals->ncols);

	for (n = 0; n < globals->Ref.nfiles; n++) {
	    Rast_get_d_row(in_fd[n], inbuf[n], row);
	}

	for (col = 0; col < globals->ncols; col++) {

	    if (!(FLAG_GET(globals->null_flag, row, col))) {
		
		Segment_get(&globals->rid_seg, (void *) &rid, row, col);

		if (rid > 0) {
		    
		    Ri.row = Rk.row = row;
		    Ri.col = Rk.col = col;

		    /* get values for Ri = this region */
		    globals->rs.id = rid;
		    fetch_reg_stats(row, col, &globals->rs, globals);
		    Ri.mean = globals->rs.mean;
		    Ri.count = globals->rs.count; 

		    sim = 0.;
		    /* region consists of more than one cell */
		    if (Ri.count > 1) {

			/* get values for Rk = this cell */
			for (n = 0; n < globals->Ref.nfiles; n++) {
			    if (globals->weighted == FALSE)
				/* scaled version */
				globals->second_val[n] = (inbuf[n][col] - min[n]) / (max[n] - min[n]);
			    else
				globals->second_val[n] = inbuf[n][col];
			}

			Rk.mean = globals->second_val;

			/* calculate similarity */
			sim = (*globals->calculate_similarity) (&Ri, &Rk, globals);
		    }
		    
		    if (0) {
			if (sim < thresh)
			    meanbuf[col] = 1;
			else {
			    sim = 1. - (sim - thresh) / maxdev;
			    meanbuf[col] = sim;
			    if (mingood > sim)
				mingood = sim;
			}
		    }
		    else {
			sim = 1 - sim;
			meanbuf[col] = sim;
			if (mingood > sim)
			    mingood = sim;
		    }
		}
	    }
	}
	Rast_put_row(mean_fd, meanbuf, FCELL_TYPE);
    }

    Rast_close(mean_fd);

    Rast_init_colors(&colors);
    Rast_make_grey_scale_fp_colors(&colors, mingood, 1);
    Rast_write_colors(globals->gof, G_mapset(), &colors);

    Rast_short_history(globals->gof, "raster", &hist);
    Rast_command_history(&hist);
    Rast_write_history(globals->gof, &hist);

    G_free(meanbuf);

    G_debug(1, "Closing input rasters...");
    for (n = 0; n < globals->Ref.nfiles; n++) {
	Rast_close(in_fd[n]);
	G_free(inbuf[n]);
    }

    G_free(inbuf);
    G_free(in_fd);
    G_free(fp_range);
    G_free(min);
    G_free(max);

    return TRUE;
}

int write_bands_ms(struct globals *globals)
{
    int *out_fd, row, col, n;
    DCELL **outbuf;
    char **name;
    struct Colors colors;
    struct History hist;
    struct ngbr_stats Rout;

    out_fd = G_malloc(sizeof(int) * globals->nbands);
    name = G_malloc(sizeof(char *) * globals->nbands);
    outbuf = G_malloc(sizeof(DCELL) * globals->nbands);
    for (n = 0; n < globals->nbands; n++) {
	outbuf[n] = Rast_allocate_d_buf();
	G_asprintf(&name[n], "%s%s", globals->Ref.file[n].name, globals->bsuf);
	out_fd[n] = Rast_open_new(name[n], DCELL_TYPE);
    }
    
    Rout.mean = G_malloc(globals->datasize);

    G_message(_("Writing out shifted band values..."));

    for (row = 0; row < globals->nrows; row++) {

	G_percent(row, globals->nrows, 9);

	for (n = 0; n < globals->nbands; n++)
	    Rast_set_d_null_value(outbuf[n], globals->ncols);
	for (col = 0; col < globals->ncols; col++) {

	    if (!(FLAG_GET(globals->null_flag, row, col))) {
		Segment_get(globals->bands_out, (void *) Rout.mean, row, col);
		
		for (n = 0; n < globals->nbands; n++) {
		    outbuf[n][col] = Rout.mean[n];
		    if (globals->weighted == FALSE)
			outbuf[n][col] = Rout.mean[n] * (globals->max[n] - globals->min[n]) + globals->min[n];
		}
	    }
	}
	for (n = 0; n < globals->nbands; n++)
	    Rast_put_row(out_fd[n], outbuf[n], DCELL_TYPE);
    }

    for (n = 0; n < globals->nbands; n++) {
	Rast_close(out_fd[n]);

	Rast_read_colors(globals->Ref.file[n].name, globals->Ref.file[n].mapset, &colors);
	Rast_write_colors(name[n], G_mapset(), &colors);

	Rast_short_history(name[n], "raster", &hist);
	Rast_command_history(&hist);
	Rast_write_history(name[n], &hist);
    }

    /* free */

    return TRUE;
}

int close_files(struct globals *globals)
{
    G_debug(1, "Closing input rasters...");

    /* close segmentation files and output raster */
    G_debug(1, "closing files");
    Segment_close(&globals->bands_seg);
    if (globals->method == ORM_MS)
	Segment_close(&globals->bands_seg2);
    if (globals->bounds_map)
	Segment_close(&globals->bounds_seg);

    G_free(globals->bands_val);
    G_free(globals->second_val);

    Segment_close(&globals->rid_seg);

    flag_destroy(globals->null_flag);
    flag_destroy(globals->candidate_flag);
    
    rgtree_destroy(globals->reg_tree);

    /* anything else left to clean up? */

    return TRUE;
}
