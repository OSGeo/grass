#include <stdlib.h>
#include <grass/gis.h>
#include "global.h"

int raw_stats(int fd[], int with_coordinates, int with_xy, int with_labels)
{
    CELL null_cell;
    void **rast, **rastp;
    char str1[50];
    register int i;
    int row, col, nulls_found;
    double G_row_to_northing(), G_col_to_easting();
    struct Cell_head window;
    char nbuf[100], ebuf[100];
    RASTER_MAP_TYPE *map_type;

    /* allocate i/o buffers for each raster map */
    rast = (void **)G_calloc(nfiles, sizeof(void *));
    rastp = (void **)G_calloc(nfiles, sizeof(void *));
    map_type = (RASTER_MAP_TYPE *) G_calloc(nfiles, sizeof(RASTER_MAP_TYPE));
    for (i = 0; i < nfiles; i++) {
	/* if fp map and report real data, not cat indexes, set type to DCELL */
	if (is_fp[i] && !raw_output && !as_int)
	    map_type[i] = DCELL_TYPE;
	else
	    map_type[i] = CELL_TYPE;
	rast[i] = G_allocate_raster_buf(map_type[i]);
    }

    /* get window */
    if (with_coordinates)
	G_get_set_window(&window);

    /* here we go */
    G_set_c_null_value(&null_cell, 1);
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);

	/* read the rows and set the pointers */
	for (i = 0; i < nfiles; i++) {
	    if (G_get_raster_row(fd[i], rast[i], row, map_type[i]) < 0)
		exit(1);
	    rastp[i] = rast[i];
	}

	if (with_coordinates)
	    G_format_northing(G_row_to_northing(row + .5, &window), nbuf, 
			      G_projection() == PROJECTION_LL ? -1 : 0);

	for (col = 0; col < ncols; col++) {
	    if (no_nulls || no_nulls_all) {
		nulls_found = 0;
		for (i = 0; i < nfiles; i++) {
		    /*
		       G_set_raster_value_d(zero_val, 0.0, map_type[i]);
		       if (G_raster_cmp(rastp[i], zero_val, map_type[i]) != 0)
		       break;
		     */
		    if (G_is_null_value(rastp[i], map_type[i]))
			nulls_found++;
		}

		if ((nulls_found == nfiles) || (nulls_found && no_nulls)) {
		    for (i = 0; i < nfiles; i++)
			rastp[i] = G_incr_void_ptr(rastp[i],
						   G_raster_size(map_type
								 [i]));
		    continue;
		}
	    }
	    if (with_coordinates) {
		G_format_easting(G_col_to_easting(col + .5, &window), ebuf,
				 G_projection() == PROJECTION_LL ? -1 : 0);
		fprintf(stdout, "%s%s%s%s", ebuf, fs, nbuf, fs);
	    }
	    if (with_xy)
		fprintf(stdout, "%d%s%d%s", col + 1, fs, row + 1, fs);

	    for (i = 0; i < nfiles; i++) {
		if (G_is_null_value(rastp[i], map_type[i])) {
		    fprintf(stdout, "%s%s", i ? fs : "", no_data_str);
		    if (with_labels)
			fprintf(stdout, "%s%s", fs,
				G_get_cat(null_cell, &labels[i]));
		}
		else if (map_type[i] == CELL_TYPE) {
		    fprintf(stdout, "%s%ld", i ? fs : "",
			    (long)*((CELL *) rastp[i]));
		    if (with_labels && !is_fp[i])
			fprintf(stdout, "%s%s", fs,
				G_get_cat(*((CELL *) rastp[i]), &labels[i]));
		}
		else {		/* floating point cell */

		    sprintf(str1, "%.10f", *((DCELL *) rastp[i]));
		    G_trim_decimal(str1);
		    G_strip(str1);
		    fprintf(stdout, "%s%s", i ? fs : "", str1);
		    if (with_labels)
			fprintf(stdout, "%s%s", fs,
				G_get_d_raster_cat((DCELL *) rastp[i],
						   &labels[i]));
		}
		rastp[i] =
		    G_incr_void_ptr(rastp[i], G_raster_size(map_type[i]));
	    }
	    fprintf(stdout, "\n");
	}
    }

    G_percent(row, nrows, 2);

    return 0;
}
