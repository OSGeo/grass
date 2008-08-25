#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"
#include "plot.h"

int label(struct Map_info *Map, int type, int do_area,
	  struct cat_list *Clist, LATTR *lattr, int chcat)
{
    int i, ltype;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int cat;
    char text[100];

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    Vect_rewind(Map);

    while (1) {
	ltype = Vect_read_next_line(Map, Points, Cats);
	switch (ltype) {
	case -1:
	    G_fatal_error(_("Can't read vector map"));
	case -2:		/* EOF */
	    return 0;
	}

	if (!(type & ltype))
	    continue;		/* used for both lines and labels */

	R_RGB_color(lattr->color.R, lattr->color.G, lattr->color.B);
	R_text_size(lattr->size, lattr->size);
	if (lattr->font)
	    R_font(lattr->font);
	if (lattr->enc)
	    R_encoding(lattr->enc);

	if (chcat) {
	    int found = 0;

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
	    int found = 0;

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

	if (Vect_cat_get(Cats, lattr->field, &cat)) {
	    text[0] = '\0';
	    for (i = 0; i < Cats->n_cats; i++) {
		G_debug(3, "cat lab: field = %d, cat = %d", Cats->field[i],
			Cats->cat[i]);
		if (Cats->field[i] == lattr->field) {	/* all cats of given lfield */
		    if (*text != '\0')
			sprintf(text, "%s/", text);

		    sprintf(text, "%s%d", text, Cats->cat[i]);
		}
	    }
	    show_label_line(Points, ltype, lattr, text);
	}
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return 0;
}
