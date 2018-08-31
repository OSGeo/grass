#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "plot.h"

/* arrow heads will be drawn at 25,50,75% of the line length */
#define PERC_OF_LINE 25

int display_dir(struct Map_info *Map, int type, struct cat_list *Clist,
		int chcat, int dsize)
{
    int ltype;
    double len, x, y, angle, msize, dist;
    struct line_pnts *Points;
    struct line_cats *Cats;

    G_debug(1, "display direction:");
    msize = dsize * (D_d_to_u_col(2.0) - D_d_to_u_col(1.0));	/* do it better */

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    Vect_rewind(Map);

    while (1) {
	ltype = Vect_read_next_line(Map, Points, Cats);
	switch (ltype) {
	case -1:
	    G_fatal_error(_("Unable to read vector map"));
	case -2:		/* EOF */
	    return 0;
	}

        if (!(ltype & type))
          continue;

	if (!(ltype & (GV_LINES | GV_FACE)))
	    continue;

	if (chcat) {
	    int i, found = 0;

	    for (i = 0; i < Cats->n_cats; i++) {
		if (Cats->field[i] == Clist->field &&
		    Vect_cat_in_cat_list(Cats->cat[i], Clist)) {
		    found = 1;
		    break;
		}
	    }
	    if (!found)
		continue;
	}
	else if (Clist->field > 0) {
	    int i, found = 0;

	    for (i = 0; i < Cats->n_cats; i++) {
		if (Cats->field[i] == Clist->field) {
		    found = 1;
		    break;
		}
	    }
	    /* lines with no category will be displayed */
	    if (Cats->n_cats > 0 && !found)
		continue;
	}

	len = Vect_line_length(Points);

	for (dist = PERC_OF_LINE / 100.0; dist <= 1.0 - PERC_OF_LINE / 100.0;
	     dist += PERC_OF_LINE / 100.0) {
	    Vect_point_on_line(Points, len * dist, &x, &y, NULL, &angle,
			       NULL);
	    G_debug(4, "plot direction: %f, %f", x, y);
	    D_plot_icon(x, y, G_ICON_ARROW, angle, msize);
	}
    }


    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return 0;			/* not reached */
}
