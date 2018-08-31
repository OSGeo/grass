/*
 **********************************************************************
 *
 * MODULE:        r.support.stats
 *
 * AUTHOR(S):     Brad Douglas <rez touchofmadness com>
 *
 * PURPOSE:       Update raster statistics
 *
 * COPYRIGHT:     (C) 2006 by the GRASS Development Team
 *
 *                This program is free software under the GNU General
 *                Purpose License (>=v2). Read the file COPYING that
 *                comes with GRASS for details.
 *
 ***********************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"


/*
 * check_stats() - Check and update statistics 
 *
 * RETURN: 0 on success / 1 on failure
 */
int check_stats(const char *name)
{
    RASTER_MAP_TYPE data_type;
    struct Histogram histogram;
    struct Categories cats;
    struct Range range;
    struct FPRange fprange;
    int i, histo_num;
    int cats_ok;
    int max;

    data_type = Rast_map_type(name, "");

    G_message(_("Updating statistics for [%s]..."), name);

    if (!do_histogram(name))
	return 1;
    if (Rast_read_histogram(name, "", &histogram) <= 0)
	return 1;

    /* Init histogram range */
    if (data_type == CELL_TYPE)
	Rast_init_range(&range);
    else
	Rast_init_fp_range(&fprange);

    G_message(_("Updating histogram range..."));
    i = histo_num = Rast_get_histogram_num(&histogram);
    while (i >= 0) {
	G_percent(i, histo_num, 2);

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
	G_message(_("Updating the number of categories for [%s]..."), name);
	Rast_write_cats(name, &cats);
    }

    Rast_free_histogram(&histogram);
    Rast_free_cats(&cats);

    return 0;
}
