#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>
#include "local_proto.h"


/*
 * check_stats() - Check and update statistics 
 *
 * RETURN: EXIT_SUCCESS / EXIT_FAILURE
 */
int check_stats(const char *name)
{
    struct Categories cats;
    struct FPRange fprange;
    int cats_ok;

    G_message(_("Updating statistics for <%s>"), name);
    
    /* Get category status and max */
    cats_ok = (Rast3d_read_cats(name, "", &cats) >= 0);
    Rast3d_read_range(name, "", &fprange);

    /* Further category checks */
    if (!cats_ok)
	Rast_init_cats("", &cats);
    else if (cats.num != fprange.max) {
	cats.num = fprange.max;
	cats_ok = 0;
    }

    /* Update categories if needed */
    if (!cats_ok) {
	G_message(_("Updating the number of categories for <%s>"), name);
	Rast3d_write_cats(name, &cats);
    }
    Rast_free_cats(&cats);

    return 0;
}
