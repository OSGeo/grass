/* CMD mode from Bob Covill 2001 
 *
 * small fixes: MN
 *
 * Bug left: extension overwrites input name 1/2002
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <grass/glocale.h>
#include "global.h"

int exec_rectify(int order, char *extension)
/* ADDED WITH CRS MODIFICATIONS */
{
    char *name;
    char *mapset;
    char *result;
    char *type;
    int i, n;
    struct Colors colr;
    struct Categories cats;
    struct History hist;
    int colr_ok, cats_ok;
    long start_time, rectify_time, compress_time;


    /* allocate the output cell matrix */
    cell_buf = (void **)G_calloc(NROWS, sizeof(void *));
    n = NCOLS * G_raster_size(map_type);
    for (i = 0; i < NROWS; i++) {
	cell_buf[i] = (void *)G_malloc(n);
	G_set_null_value(cell_buf[i], NCOLS, map_type);
    }

    /* rectify each file */
    for (n = 0; n < ref.nfiles; n++) {
	if ((i = ref_list[n]) < 0) {
	    /* continue; */
	    name = ref.file[n].name;
	    mapset = ref.file[n].mapset;

	    /* generate out name, add extension to output */
	    result =
		G_malloc(strlen(ref.file[n].name) + strlen(extension) + 1);
	    strcpy(result, ref.file[n].name);
	    strcat(result, extension);
	    G_message(_("Rectified input raster map <%s> will be saved as <%s>"),
		      name, result);

	    select_current_env();

	    cats_ok = G_read_cats(name, mapset, &cats) >= 0;
	    colr_ok = G_read_colors(name, mapset, &colr) > 0;

	    /* Initialze History */
	    type = "raster";
	    G_short_history(name, type, &hist);

	    time(&start_time);

	    if (rectify(name, mapset, result, order)) {
		select_target_env();

	    /***
	     * This clobbers (with wrong values) head
	     * written by gislib.  99% sure it should
	     * be removed.  EGM 2002/01/03
            G_put_cellhd (result,&target_window);
	     */
		if (cats_ok) {
		    G_write_cats(result, &cats);
		    G_free_cats(&cats);
		}
		if (colr_ok) {
		    G_write_colors(result, G_mapset(), &colr);
		    G_free_colors(&colr);
		}

		/* Write out History Structure History */
		sprintf(hist.title, "%s", result);
		sprintf(hist.datsrc_1, "%s", name);
		sprintf(hist.edhist[0], "Created from: i.rectify");
		sprintf(hist.edhist[1], "Transformation order = %d", order);
		hist.edlinecnt = 2;
		G_write_history(result, &hist);

		select_current_env();
		time(&rectify_time);
		compress_time = rectify_time;
		report(name, mapset, result, rectify_time - start_time,
		       compress_time - rectify_time, 1);
	    }
	    else
		report(name, mapset, result, (long)0, (long)0, 0);
	}
    }

    G_done_msg("");

    return 0;
}
