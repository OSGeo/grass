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
    struct Categories cats;
    struct Range range;
    struct FPRange fprange;
    int cats_ok;
    int max;

    data_type = Rast_map_type(name, "");

    G_message(_("\n  Updating statistics for [%s]"), name);
    if (do_histogram(name) < 0)
	return 0;

    /* get range */
    if (data_type == CELL_TYPE)
	Rast_read_range(name, "", &range);
    else
	Rast_read_fp_range(name, "", &fprange);
    max = (data_type == CELL_TYPE ? range.max : fprange.max);

    /* Get category status */
    cats_ok = (Rast_read_cats(name, "", &cats) >= 0);

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
	Rast_free_cats(&cats);
    }

    return 0;
}
