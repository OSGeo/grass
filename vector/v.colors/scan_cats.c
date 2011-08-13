#include <grass/vector.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

static void scan_layer(int, const struct line_cats *, int *, int *);

void scan_cats(const struct Map_info *Map, int field, const char *style,
	       struct Colors *colors, int *cmin, int *cmax)
{
    int ltype, lmin, lmax;
    struct line_cats *Cats;

    *cmin = *cmax = -1;
    Cats = Vect_new_cats_struct();

    while(TRUE) {
	ltype = Vect_read_next_line(Map, NULL, Cats);
	if (ltype == -1)
	    G_fatal_error(_("Unable to read vector map"));
	if (ltype == -2)
	    break; /* EOF */

	scan_layer(field, Cats, &lmin, &lmax);

	if (*cmin == -1 || lmin <= *cmin)
	    *cmin = lmin;
	if (*cmax == -1 || lmax >= *cmax)
	    *cmax = lmax;
    }

    Rast_make_colors(colors, style, (CELL) *cmin, (CELL) *cmax);

    Vect_destroy_cats_struct(Cats);
}

void scan_layer(int field, const struct line_cats *Cats, int *cmin, int *cmax)
{
    int n, cat;

    *cmin = *cmax = -1;
    for (n = 0; n < Cats->n_cats; n++) {
        if (Cats->field[n] == field) {
	    cat = Cats->cat[n];
	    if (*cmin == -1 || cat <= *cmin)
		*cmin = cat;
	    if (*cmax == -1 || cat >= *cmax)
		*cmax = cat;
        }
    }
}
