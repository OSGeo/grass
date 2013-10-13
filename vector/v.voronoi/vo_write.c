#include <stdio.h>
#include <math.h>
#include <float.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/display.h>
#include "sw_defs.h"
#include "defs.h"

static int write_skeleton(struct line_pnts *Points)
{
    int i, area1, area2;
    static struct line_pnts *APoints = NULL;
    static struct line_cats *Cats = NULL;

    if (!APoints) {
	APoints = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();
    }

    if ((area1 = Vect_find_area(&In, Points->x[0], Points->y[0])) == 0)
	return 0;
    if ((area2 = Vect_find_area(&In, Points->x[1], Points->y[1])) == 0)
	return 0;
    if (area1 != area2)
	return 0;

    if (!Vect_get_area_centroid(&In, area1))
	return 0;
    Vect_get_area_points(&In, area1, APoints);
    if (Vect_line_check_intersection(Points, APoints, 0))
	return 0;

    for (i = 0; i < Vect_get_area_num_isles(&In, area1); i++) {
	Vect_get_isle_points(&In, Vect_get_area_isle(&In, area1, i), APoints);
	if (Vect_line_check_intersection(Points, APoints, 0))
	    return 0;
    }

    Vect_get_area_cats(&In, area1, Cats);
    Vect_write_line(&Out, GV_LINE, Points, Cats);

    return 1;
}

int vo_write(void)
{
    struct Halfedge *lbnd;

    for (lbnd = ELright(ELleftend); lbnd != ELrightend; lbnd = ELright(lbnd)) {
	write_ep(lbnd->ELedge);
    }

    /* TODO: free memory */

    return 1;
}


int write_ep(struct Edge *e)
{
    static struct line_pnts *Points = NULL;
    static struct line_cats *Cats = NULL;
    double x1, y1, x2, y2;

    if (!Points) {
	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();
    }

    if (in_area && e->reg[le]->sitenbr == e->reg[re]->sitenbr)
	return 0;

    if (e->ep[le] != NULL && e->ep[re] != NULL) {	/* both end defined */
	x1 = e->ep[le]->coord.x;
	y1 = e->ep[le]->coord.y;
	x2 = e->ep[re]->coord.x;
	y2 = e->ep[re]->coord.y;

	if (!Vect_point_in_box(x1, y1, 0.0, &Box) ||
	    !Vect_point_in_box(x2, y2, 0.0, &Box)) {
	    Vect_box_clip(&x1, &y1, &x2, &y2, &Box);
	}

	/* Don't write zero length */
	if (x1 == x2 && y1 == y2)
	    return 0;

	Vect_reset_line(Points);
	Vect_append_point(Points, x1, y1, 0.0);
	Vect_append_point(Points, x2, y2, 0.0);
	if (skeleton)
	    write_skeleton(Points);
	else
	    Vect_write_line(&Out, Type, Points, Cats);
    }
    else {
	int knownPointAtLeft = -1;

	if (e->ep[le] != NULL) {
	    x1 = e->ep[le]->coord.x;
	    y1 = e->ep[le]->coord.y;
	    knownPointAtLeft = 1;
	}
	else if (e->ep[re] != NULL) {
	    x1 = e->ep[re]->coord.x;
	    y1 = e->ep[re]->coord.y;
	    knownPointAtLeft = 0;
	}

	if (knownPointAtLeft == -1) {
	    x2 = (e->reg[le]->coord.x + e->reg[re]->coord.x) / 2.0;
	    y2 = (e->reg[le]->coord.y + e->reg[re]->coord.y) / 2.0;
	    knownPointAtLeft = 0;
	    if (!extend_line(Box.S, Box.N, Box.W, Box.E,
			     e->a, e->b, e->c, x2, y2, &x1, &y1,
			     knownPointAtLeft)) {
				 
		G_warning("Undefined edge, unable to extend line");
		
		return 0;
	    }
	    knownPointAtLeft = 1;
	}
	if (extend_line(Box.S, Box.N, Box.W, Box.E,
			e->a, e->b, e->c, x1, y1, &x2, &y2,
			knownPointAtLeft)) {

	    /* Don't write zero length */
	    if (x1 == x2 && y1 == y2)
		return 0;

	    Vect_reset_line(Points);
	    Vect_append_point(Points, x1, y1, 0.0);
	    Vect_append_point(Points, x2, y2, 0.0);
	    if (skeleton)
		write_skeleton(Points);
	    else
		Vect_write_line(&Out, Type, Points, Cats);
	}
    }

    return 0;
}
