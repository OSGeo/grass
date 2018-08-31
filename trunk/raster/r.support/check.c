#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"


/*
 * check_stats() - Check and update statistics 
 *
 * RETURN: EXIT_SUCCESS / EXIT_FAILURE
 */
int check_stats(const char *name)
{
    RASTER_MAP_TYPE data_type;
    struct Histogram histogram;
    struct Categories cats;
    struct Range range;
    struct FPRange fprange;
    int i;
    int cats_ok;
    int max;

    data_type = Rast_map_type(name, "");

    G_message(_("\n  Updating statistics for [%s]"), name);
    if (do_histogram(name) < 0)
	return 0;

    if (Rast_read_histogram(name, "", &histogram) <= 0)
	return 0;

    /* Init histogram range */
    if (data_type == CELL_TYPE)
	Rast_init_range(&range);
    else
	Rast_init_fp_range(&fprange);

    /* Update histogram range */
    i = Rast_get_histogram_num(&histogram);
    while (i >= 0) {
	if (data_type == CELL_TYPE)
	    Rast_update_range(Rast_get_histogram_cat(i--, &histogram), &range);
	else
	    Rast_update_fp_range((DCELL) Rast_get_histogram_cat(i--, &histogram),
			      &fprange);
    }

    /* Write histogram range */
    if (data_type == CELL_TYPE)
	Rast_write_range(name, &range);
    else
	Rast_write_fp_range(name, &fprange);

    /* Get category status and max */
    cats_ok = (Rast_read_cats(name, "", &cats) >= 0);
    max = (data_type == CELL_TYPE ? range.max : fprange.max);

    /* Further category checks */
    if (!cats_ok)
	Rast_init_cats("", &cats);
    else if (cats.num != max) {
	cats.num = max;
	cats_ok = 0;
    }

    /* Update categories if needed */
    if (!cats_ok) {
	G_message(_("   Updating the number of categories for "
		    "[%s]\n\n"), name);
	Rast_write_cats(name, &cats);
    }

    Rast_free_histogram(&histogram);
    Rast_free_cats(&cats);

    return 0;
}
