#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "global.h"

int raw_stats(int fd[], int with_coordinates, int with_xy, int with_labels)
{
    CELL null_cell;
    void **rast, **rastp;
    char str1[50];
    register int i;
    int row, col, nulls_found;
    double Rast_row_to_northing(), G_col_to_easting();
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
	    map_type[i] = Rast_get_map_type(fd[i]);
	else
	    map_type[i] = CELL_TYPE;

	rast[i] = Rast_allocate_buf(map_type[i]);
    }

    /* get window */
    if (with_coordinates)
	G_get_set_window(&window);

    /* here we go */
    Rast_set_c_null_value(&null_cell, 1);
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);

	/* read the rows and set the pointers */
	for (i = 0; i < nfiles; i++) {
	    Rast_get_row(fd[i], rast[i], row, map_type[i]);
	    rastp[i] = rast[i];
	}

	if (with_coordinates)
	    G_format_northing(Rast_row_to_northing(row + .5, &window), nbuf, 
			      G_projection() == PROJECTION_LL ? -1 : 0);

	for (col = 0; col < ncols; col++) {
	    if (no_nulls || no_nulls_all) {
		nulls_found = 0;
		for (i = 0; i < nfiles; i++) {
		    /*
		       Rast_set_d_value(zero_val, 0.0, map_type[i]);
		       if (Rast_raster_cmp(rastp[i], zero_val, map_type[i]) != 0)
		       break;
		     */
		    if (Rast_is_null_value(rastp[i], map_type[i]))
			nulls_found++;
		}

		if ((nulls_found == nfiles) || (nulls_found && no_nulls)) {
		    for (i = 0; i < nfiles; i++)
			rastp[i] = G_incr_void_ptr(rastp[i],
						   Rast_cell_size(map_type
								 [i]));
		    continue;
		}
	    }
	    if (with_coordinates) {
		G_format_easting(Rast_col_to_easting(col + .5, &window), ebuf,
				 G_projection() == PROJECTION_LL ? -1 : 0);
		fprintf(stdout, "%s%s%s%s", ebuf, fs, nbuf, fs);
	    }
	    if (with_xy)
		fprintf(stdout, "%d%s%d%s", col + 1, fs, row + 1, fs);

	    for (i = 0; i < nfiles; i++) {
		if (Rast_is_null_value(rastp[i], map_type[i])) {
		    fprintf(stdout, "%s%s", i ? fs : "", no_data_str);
		    if (with_labels)
			fprintf(stdout, "%s%s", fs,
				Rast_get_c_cat(&null_cell, &labels[i]));
		}
		else if (map_type[i] == CELL_TYPE) {
		    fprintf(stdout, "%s%ld", i ? fs : "",
			    (long)*((CELL *) rastp[i]));
		    if (with_labels && !is_fp[i])
			fprintf(stdout, "%s%s", fs,
				Rast_get_c_cat((CELL *) rastp[i], &labels[i]));
		}
		else if (map_type[i] == FCELL_TYPE) {
		    sprintf(str1, "%.8g", *((FCELL *) rastp[i]));
		    G_trim_decimal(str1);
		    G_strip(str1);
		    fprintf(stdout, "%s%s", i ? fs : "", str1);
		    if (with_labels)
			fprintf(stdout, "%s%s", fs,
				Rast_get_f_cat((FCELL *) rastp[i],
						   &labels[i]));
		}
		else if (map_type[i] == DCELL_TYPE) {
		    sprintf(str1, "%.16g", *((DCELL *) rastp[i]));
		    G_trim_decimal(str1);
		    G_strip(str1);
		    fprintf(stdout, "%s%s", i ? fs : "", str1);
		    if (with_labels)
			fprintf(stdout, "%s%s", fs,
				Rast_get_d_cat((DCELL *) rastp[i],
						   &labels[i]));
		}
		else
		    G_fatal_error(_("Invalid map type"));

		rastp[i] =
		    G_incr_void_ptr(rastp[i], Rast_cell_size(map_type[i]));
	    }
	    fprintf(stdout, "\n");
	}
    }

    G_percent(row, nrows, 2);

    return 0;
}
