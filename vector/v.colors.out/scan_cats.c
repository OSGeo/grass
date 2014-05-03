#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

static void scan_layer(int, const struct line_cats *, int *, int *);

void scan_cats(const char *name, const char *layer,
	       int *cmin, int *cmax)
{
    int ltype, lmin, lmax, ilayer;

    struct Map_info Map;
    struct line_cats *Cats;

    *cmin = *cmax = -1;

    Vect_set_open_level(1); /* no topology required */
    if (Vect_open_old2(&Map, name, "", layer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), name);

    ilayer = Vect_get_field_number(&Map, layer);
    if (ilayer < 1)
	G_fatal_error(_("Layer <%s> not found"), layer);

    Cats = Vect_new_cats_struct();

    G_message(_("Reading features..."));
    while(TRUE) {
	ltype = Vect_read_next_line(&Map, NULL, Cats);
	if (ltype == -1)
	    G_fatal_error(_("Unable to read vector map"));
	if (ltype == -2)
	    break; /* EOF */

	scan_layer(ilayer, Cats, &lmin, &lmax);

	if (*cmin == -1 || lmin <= *cmin)
	    *cmin = lmin;
	if (*cmax == -1 || lmax >= *cmax)
	    *cmax = lmax;
    }

    Vect_destroy_cats_struct(Cats);

    Vect_close(&Map);
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
