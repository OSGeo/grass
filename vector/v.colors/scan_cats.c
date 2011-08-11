#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

static void scan_layer(int, const struct line_cats *, int *, int *);

void scan_cats(const struct Map_info *Map, int field, int *min, int* max)
{
    int ltype, cmin, cmax;
    struct line_cats *Cats;

    *min = *max = -1;
    
    Cats = Vect_new_cats_struct();

    while(TRUE) {
	ltype = Vect_read_next_line(Map, NULL, Cats);
	if (ltype == -1)
	    G_fatal_error(_("Unable to read vector map"));
	if (ltype == -2)
	    break; /* EOF */

	scan_layer(field, Cats, &cmin, &cmax);

	if (*min == -1 || cmin <= *min)
	    *min = cmin;
	if (*max == -1 || cmax >= *max)
	    *max = cmax;
    }

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
