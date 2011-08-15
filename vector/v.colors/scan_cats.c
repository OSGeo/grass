#include <grass/vector.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

static void scan_layer(int, const struct line_cats *, int *, int *);

void scan_cats(const struct Map_info *Map, int field, const char *style,
	       const struct FPRange *range, struct Colors *colors, int *cmin, int *cmax)
{
    int ltype, lmin, lmax;
    struct line_cats *Cats;

    *cmin = *cmax = -1;
    Cats = Vect_new_cats_struct();

    G_message(_("Reading features..."));
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

    if (range) {
	if (range->min >= *cmin && range->min <= *cmax)
	    *cmin = range->min;
	else
	    G_warning(_("Min value (%d) is out of range %d,%d"),
		      (int) range->min, *cmin, *cmax);
	
	if (range->max <= *cmax && range->max >= *cmin)
	    *cmax = range->max;
	else
	    G_warning(_("Max value (%d) is out of range %d,%d"),
		      (int) range->max, *cmin, *cmax);
    }
    
    make_colors(colors, style, (DCELL) *cmin, (DCELL) *cmax, FALSE);

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
