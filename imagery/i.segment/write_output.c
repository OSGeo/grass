/* transfer the segmented regions from the segmented data file to a raster file */
/* close_files() function is at bottom */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/segment.h>	/* segmentation library */
#include <grass/glocale.h>
#include "iseg.h"

/* TODO some time delay here with meanbuf, etc being processed.  I only put if statements on the actual files
 * to try and keep the code more clear.  Need to see if this raster makes stats processing easier?  Or IFDEF it out?
 */

int write_output(struct globals *globals)
{
    int out_fd, row, col;
    CELL *outbuf, rid;
    struct Colors colors;
    struct History hist;

    outbuf = Rast_allocate_c_buf();

    G_debug(1, "preparing output raster");
    /* open output raster map */
    out_fd = Rast_open_new(globals->out_name, CELL_TYPE);

    G_debug(1, "start data transfer from segmentation file to raster");

    G_message(_("Writing out segment IDs"));
    for (row = 0; row < globals->nrows; row++) {

	G_percent(row, globals->nrows, 9);

	Rast_set_c_null_value(outbuf, globals->ncols);
	for (col = 0; col < globals->ncols; col++) {

	    if (!(FLAG_GET(globals->null_flag, row, col))) {
		segment_get(&globals->rid_seg, (void *) &rid, row, col);

		if (rid > 0) {
		    outbuf[col] = rid;
		}
	    }
	}
	Rast_put_row(out_fd, outbuf, CELL_TYPE);
    }

    /* close and save segment id file */
    Rast_close(out_fd);
    G_free(outbuf);

    /* set colors */
    Rast_init_colors(&colors);
    Rast_make_random_colors(&colors, 1, globals->nrows * globals->ncols);
    Rast_write_colors(globals->out_name, G_mapset(), &colors);

    Rast_short_history(globals->out_name, "raster", &hist);
    Rast_command_history(&hist);
    Rast_write_history(globals->out_name, &hist);

    /* write goodness of fit */
    if (globals->out_band) {
	int mean_fd;
	FCELL *meanbuf;
	double thresh, maxdev, sim, mingood;
	struct ngbr_stats Ri, Rk;
	struct Ref Ref;		/* group reference list */
	DCELL **inbuf;		/* buffers to store lines from each of the imagery group rasters */
	int n, *in_fd;

	mean_fd = Rast_open_new(globals->out_band, FCELL_TYPE);
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
	if (!I_get_group_ref(globals->image_group, &Ref))
	    G_fatal_error(_("Unable to read REF file for group <%s>"),
			  globals->image_group);
	if (Ref.nfiles <= 0)
	    G_fatal_error(_("Group <%s> contains no raster maps"),
			  globals->image_group);

	in_fd = G_malloc(Ref.nfiles * sizeof(int));
	inbuf = (DCELL **) G_malloc(Ref.nfiles * sizeof(DCELL *));

	G_debug(1, "Opening input rasters...");
	for (n = 0; n < Ref.nfiles; n++) {
	    inbuf[n] = Rast_allocate_d_buf();
	    in_fd[n] = Rast_open_old(Ref.file[n].name, Ref.file[n].mapset);
	}

	G_message(_("Writing out goodness of fit"));
	for (row = 0; row < globals->nrows; row++) {

	    G_percent(row, globals->nrows, 9);

	    Rast_set_f_null_value(meanbuf, globals->ncols);

	    for (n = 0; n < Ref.nfiles; n++) {
		Rast_get_d_row(in_fd[n], inbuf[n], row);
	    }

	    for (col = 0; col < globals->ncols; col++) {

		if (!(FLAG_GET(globals->null_flag, row, col))) {
		    
		    segment_get(&globals->rid_seg, (void *) &rid, row, col);

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
			    for (n = 0; n < Ref.nfiles; n++) {
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
			    sim = 1 - sim / globals->max_diff;
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
	Rast_write_colors(globals->out_band, G_mapset(), &colors);

	Rast_short_history(globals->out_band, "raster", &hist);
	Rast_command_history(&hist);
	Rast_write_history(globals->out_band, &hist);

	G_free(meanbuf);

	G_debug(1, "Closing input rasters...");
	for (n = 0; n < Ref.nfiles; n++) {
	    Rast_close(in_fd[n]);
	    G_free(inbuf[n]);
	}
	G_free(inbuf);
	G_free(in_fd);
    }

    /* free memory */
    Rast_free_colors(&colors);

    return TRUE;
}

int close_files(struct globals *globals)
{
    /* close segmentation files and output raster */
    G_debug(1, "closing files");
    segment_close(&globals->bands_seg);
    if (globals->bounds_map)
	segment_close(&globals->bounds_seg);

    G_free(globals->bands_val);
    G_free(globals->second_val);

    segment_close(&globals->rid_seg);

    flag_destroy(globals->null_flag);
    flag_destroy(globals->candidate_flag);
    
    rgtree_destroy(globals->reg_tree);

    /* anything else left to clean up? */

    return TRUE;
}
