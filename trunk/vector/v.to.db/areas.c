#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/glocale.h>

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
    struct bound_box Bbox;
    double area, perimeter;

    Cats = Vect_new_cats_struct();
    nareas = Vect_get_num_areas(Map);

    G_message(_("Reading areas..."));

    /* Cycle through all areas */
    for (area_num = 1; area_num <= nareas; area_num++) {
	area = 0;
	perimeter = 0;

	if ((options.option == O_COMPACT) || (options.option == O_FD) ||
	    (options.option == O_AREA)) {
	    area = Vect_get_area_area(Map, area_num);
	}
	if ((options.option == O_COMPACT) || (options.option == O_FD) ||
	    (options.option == O_PERIMETER)) {
	    perimeter = Vect_get_area_perimeter(Map, area_num);
	    if (G_projection() != PROJECTION_LL && G_projection() != PROJECTION_XY)
		perimeter = perimeter * G_database_units_to_meters_factor();
	}
	if (options.option == O_BBOX) {
	    Vect_get_area_box(Map, area_num, &Bbox);
	}

	found = 0;
	if (Vect_get_area_cats(Map, area_num, Cats) == 0) {
	    for (i = 0; i < Cats->n_cats; i++) {
		if (Cats->field[i] == options.field) {
		    idx = find_cat(Cats->cat[i], 1);
		    switch (options.option) {
		    case O_AREA:
			Values[idx].d1 += area;
			break;
		    case O_PERIMETER:
			Values[idx].d1 += perimeter;
			break;
		    case O_COMPACT:
		    case O_FD:
			Values[idx].d1 += area;
			Values[idx].d2 += perimeter;
			break;
		    case O_BBOX:
			if (Values[idx].d1 < Bbox.N) 
			    Values[idx].d1 = Bbox.N;
			if (Values[idx].d2 > Bbox.S) 
			    Values[idx].d2 = Bbox.S;
			if (Values[idx].d3 < Bbox.E) 
			    Values[idx].d3 = Bbox.E;
			if (Values[idx].d4 > Bbox.W) 
			    Values[idx].d4 = Bbox.W;
			break;
		    }
		    found = 1;
		}
	    }

	    if (!found) {	/* Values for no category (cat = -1) are reported at the end */
		idx = find_cat(-1, 1);
		switch (options.option) {
		case O_AREA:
		    Values[idx].d1 += area;
		    break;
		case O_PERIMETER:
		    Values[idx].d1 += perimeter;
		    break;
		case O_COMPACT:
		case O_FD:
		    Values[idx].d1 += area;
		    Values[idx].d2 += perimeter;
		    break;
	        case O_BBOX:
		    if (Values[idx].d1 < Bbox.N) 
			Values[idx].d1 = Bbox.N;
		    if (Values[idx].d2 > Bbox.S) 
			Values[idx].d2 = Bbox.S;
		    if (Values[idx].d3 < Bbox.E) 
			Values[idx].d3 = Bbox.E;
		    if (Values[idx].d4 > Bbox.W) 
			Values[idx].d4 = Bbox.W;
		    break;
		}
	    }
	}
	G_percent(area_num, nareas, 2);
    }

    return 0;
}
