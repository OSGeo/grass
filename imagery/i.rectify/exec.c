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
#include <math.h>

#include <grass/raster.h>
#include <grass/glocale.h>

#include "global.h"

int exec_rectify(int order, char *extension, char *interp_method)
/* ADDED WITH CRS MODIFICATIONS */
{
    char *name;
    char *mapset;
    char *result;
    char *type = "raster";
    int n;
    struct Colors colr;
    struct Categories cats;
    struct History hist;
    int colr_ok, cats_ok;
    long start_time, rectify_time;

    Rast_set_output_window(&target_window);
    G_message("-----------------------------------------------");

    /* rectify each file */
    for (n = 0; n < ref.nfiles; n++) {
	if (ref_list[n]) {
	    name = ref.file[n].name;
	    mapset = ref.file[n].mapset;

	    /* generate out name, add extension to output */
	    result =
		G_malloc(strlen(ref.file[n].name) + strlen(extension) + 1);
	    strcpy(result, ref.file[n].name);
	    strcat(result, extension);

	    select_current_env();

	    cats_ok = Rast_read_cats(name, mapset, &cats) >= 0;
	    colr_ok = Rast_read_colors(name, mapset, &colr) > 0;

	    /* Initialze History */
	    if (Rast_read_history(name, mapset, &hist) < 0)
		Rast_short_history(result, type, &hist);

	    time(&start_time);

	    if (rectify(name, mapset, result, order, interp_method)) {
		select_target_env();

		if (cats_ok) {
		    Rast_write_cats(result, &cats);
		    Rast_free_cats(&cats);
		}
		if (colr_ok) {
		    Rast_write_colors(result, G_mapset(), &colr);
		    Rast_free_colors(&colr);
		}

		/* Write out History */
		Rast_command_history(&hist);
		Rast_write_history(result, &hist);

		select_current_env();
		time(&rectify_time);
		report(rectify_time - start_time, 1);
	    }
	    else
		report((long)0, 0);

	    G_free(result);
	}
    }

    return 0;
}
