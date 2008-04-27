#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "global.h"

/* This function now does one of 3 things:
 * 1) It reads the areas of the areas.
 * 2) It reads the perimeter lengths of the areas. If projection is LL, the geodesic distance is used.
 * 3) It calculates the compactness using this formula:
 *    compactness = perimeter / (2 * sqrt(M_PI * area))
 * 4) It calculates the fractal dimension of the bounding curve:
 *    D_L  = 2 * log(perimeter) / log(area)
 *    (See B.B. Mandelbrot, The Fractal Geometry of Nature. 1982.)
 */
int read_areas(struct Map_info *Map)
{
    int i, idx, found;
    int area_num, nareas;
    struct line_cats *Cats;
    struct line_pnts *Ppoints;
    double area, perimeter;

    Cats = Vect_new_cats_struct();
    nareas = Vect_get_num_areas(Map);
    /* Cycle through all areas */
    for (area_num = 1; area_num <= nareas; area_num++) {
	Ppoints = Vect_new_line_struct();
	area = 0;
	perimeter = 0;

	if ((options.option == O_COMPACT) || (options.option == O_FD) || (options.option == O_AREA)) {
	    area = Vect_get_area_area(Map, area_num);
	}
	if ((options.option == O_COMPACT) || (options.option == O_FD) || (options.option == O_PERIMETER)) {
	    Vect_get_area_points(Map, area_num, Ppoints);
	    perimeter = Vect_line_geodesic_length(Ppoints);
	}

	found = 0;
	if (Vect_get_area_cats(Map, area_num, Cats) == 0) {
	    for (i = 0; i < Cats->n_cats; i++) {
		if (Cats->field[i] == options.field) {
		    idx = find_cat(Cats->cat[i]);
		    switch (options.option) {
		    case O_AREA:
			Values[idx].d1 += area;
			break;
		    case O_PERIMETER:
			Values[idx].d1 += perimeter;
			break;
		    case O_COMPACT:
			Values[idx].d1 = perimeter / (2.0 * sqrt(M_PI * area));
			break;
		    case O_FD:
			Values[idx].d1 = 2.0 * log(perimeter) / log(area) ;
			break;
		    }
		    found = 1;
		}
	    }
	    /* why do we do this? */
	    if (!found) {	/* no category found */
		idx = find_cat(0);
		if (options.option == O_AREA) {
		    Values[idx].d1 += area;
		}
		else if (options.option == O_PERIMETER) {
		    Values[idx].d1 += area;
		}
	    }
	}
    }

    return 0;
}
