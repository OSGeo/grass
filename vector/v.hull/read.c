#include <grass/gis.h>

#include "hull.h"

int loadSiteCoordinates(struct Map_info *Map, struct Point **points, int all,
			struct Cell_head *window, int field)
{
    int i, pointIdx;
    struct line_pnts *sites;
    struct line_cats *cats;
    struct bound_box box;
    int cat, type;

    sites = Vect_new_line_struct();
    cats = Vect_new_cats_struct();

    *points = NULL;
    pointIdx = 0;
    
    /* copy window to box */
    Vect_region_box(window, &box);

    while ((type = Vect_read_next_line(Map, sites, cats)) > -1) {

	if (type != GV_POINT && !(type & GV_LINES))
	    continue;

	if (field != -1 && Vect_cat_get(cats, field, &cat) == 0)
	    continue;
	
	for (i = 0; i < sites->n_points; i++) {
	    G_debug(4, "Point: %f|%f|%f|#%d", sites->x[i], sites->y[i],
		    sites->z[i], cat);
	    
	    if (!all && !Vect_point_in_box(sites->x[i], sites->y[i], sites->z[i], &box))
		continue;
	    
	    G_debug(4, "Point in the box");

	    if ((pointIdx % ALLOC_CHUNK) == 0)
		*points = (struct Point *) G_realloc(*points,
						     (pointIdx + ALLOC_CHUNK) * sizeof(struct Point));
	    
	    (*points)[pointIdx].x = sites->x[i];
	    (*points)[pointIdx].y = sites->y[i];
	    (*points)[pointIdx].z = sites->z[i];
	    pointIdx++;
	}
    }

    if (pointIdx > 0)
	*points = (struct Point *)G_realloc(*points,
					    (pointIdx + 1) * sizeof(struct Point));
    
    return pointIdx;
}
