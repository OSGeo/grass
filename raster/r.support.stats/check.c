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

    data_type = G_raster_map_type(name, "");

    G_message(_("Updating statistics for [%s]..."), name);

    if (!do_histogram(name))
	return 1;
    if (G_read_histogram(name, "", &histogram) <= 0)
	return 1;

    /* Init histogram range */
    if (data_type == CELL_TYPE)
	G_init_range(&range);
    else
	G_init_fp_range(&fprange);

    G_message(_("Updating histogram range..."));
    i = histo_num = G_get_histogram_num(&histogram);
    while (i >= 0) {
	G_percent(i, histo_num, 2);

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
    cats_ok = (G_read_cats(name, "", &cats) >= 0);
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
	G_message(_("Updating the number of categories for [%s]..."), name);
	G_write_cats(name, &cats);
    }

    G_free_histogram(&histogram);
    G_free_cats(&cats);

    return 0;
}
