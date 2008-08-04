#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"


/*
 * check_stats() - Check and update statistics 
 *
 * RETURN: EXIT_SUCCESS / EXIT_FAILURE
 */
int check_stats(char *name, char *mapset)
{
    RASTER_MAP_TYPE data_type;
    struct Histogram histogram;
    struct Categories cats;
    struct Range range;
    struct FPRange fprange;
    int i;
    int cats_ok;
    int max;
    char question[100];

    /* NOTE: G_yes() return value 1 = hitreturn(), 0 otherwise */

    data_type = G_raster_map_type(name, mapset);

    /* Exit if not updating statistics */
    G_snprintf(question, sizeof(question), _("Update the statistics "
					     "(histogram, range) for [%s]? "),
	       name);
    if (!G_yes(question, 0))
	return EXIT_FAILURE;

    G_message(_("\n  Updating statistics for [%s]"), name);
    if (!do_histogram(name, mapset))
	return EXIT_SUCCESS;
    if (G_read_histogram(name, mapset, &histogram) <= 0)
	return EXIT_SUCCESS;

    /* Init histogram range */
    if (data_type == CELL_TYPE)
	G_init_range(&range);
    else
	G_init_fp_range(&fprange);

    /* Update histogram range */
    i = G_get_histogram_num(&histogram);
    while (i >= 0) {
	if (data_type == CELL_TYPE)
	    G_update_range(G_get_histogram_cat(i--, &histogram), &range);
	else
	    G_update_fp_range((DCELL) G_get_histogram_cat(i--, &histogram),
			      &fprange);
    }

    /* Write histogram range */
    if (data_type == CELL_TYPE)
	G_write_range(name, &range);
    else
	G_write_fp_range(name, &fprange);

    /* Get category status and max */
    cats_ok = (G_read_cats(name, mapset, &cats) >= 0);
    max = (data_type == CELL_TYPE ? range.max : fprange.max);

    /* Further category checks */
    if (!cats_ok)
	G_init_cats(max, "", &cats);
    else if (cats.num != max) {
	cats.num = max;
	cats_ok = 0;
    }

    /* Update categories if needed */
    if (!cats_ok) {
	G_message(_("   Updating the number of categories for "
		    "[%s]\n\n"), name);
	G_write_cats(name, &cats);
    }

    G_free_histogram(&histogram);
    G_free_cats(&cats);

    return EXIT_SUCCESS;
}
