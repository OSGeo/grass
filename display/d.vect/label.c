#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "local_proto.h"
#include "plot.h"

int display_label(struct Map_info *Map, int type,
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
	  G_fatal_error(_("Unable to read vector map"));
	case -2:		/* EOF */
	    return 0;
	}

	if (!(type & ltype))
	    continue;		/* used for both lines and labels */

	D_RGB_color(lattr->color.R, lattr->color.G, lattr->color.B);
	D_text_size(lattr->size, lattr->size);
	if (lattr->font)
	    D_font(lattr->font);
	if (lattr->enc)
	    D_encoding(lattr->enc);

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

void show_label(double *px, double *py, LATTR *lattr, const char *text)
{
    double X = *px, Y = *py;
    int Xoffset, Yoffset;
    double xarr[5], yarr[5];
    double T, B, L, R;

    X = X + D_get_d_to_u_xconv() * 0.5 * lattr->size;
    Y = Y + D_get_d_to_u_yconv() * 1.5 * lattr->size;

    D_pos_abs(X, Y);
    D_get_text_box(text, &T, &B, &L, &R);

    /* Expand border 1/2 of text size */
    T = T - D_get_d_to_u_yconv() * lattr->size / 2;
    B = B + D_get_d_to_u_yconv() * lattr->size / 2;
    L = L - D_get_d_to_u_xconv() * lattr->size / 2;
    R = R + D_get_d_to_u_xconv() * lattr->size / 2;

    Xoffset = 0;
    Yoffset = 0;
    if (lattr->xref == LCENTER)
	Xoffset = -(R - L) / 2;
    if (lattr->xref == LRIGHT)
	Xoffset = -(R - L);
    if (lattr->yref == LCENTER)
	Yoffset = -(B - T) / 2;
    if (lattr->yref == LBOTTOM)
	Yoffset = -(B - T);

    if (lattr->has_bgcolor || lattr->has_bcolor) {
	xarr[0] = xarr[1] = xarr[4] = L + Xoffset;
	xarr[2] = xarr[3] = R + Xoffset;
	yarr[0] = yarr[3] = yarr[4] = B + Yoffset;
	yarr[1] = yarr[2] = T + Yoffset;

	if (lattr->has_bgcolor) {
	    D_RGB_color(lattr->bgcolor.R, lattr->bgcolor.G,
			lattr->bgcolor.B);
	    D_polygon_abs(xarr, yarr, 5);
	}

	if (lattr->has_bcolor) {
	    D_RGB_color(lattr->bcolor.R, lattr->bcolor.G,
			lattr->bcolor.B);
	    D_polyline_abs(xarr, yarr, 5);
	}
	D_RGB_color(lattr->color.R, lattr->color.G, lattr->color.B);
    }

    D_pos_abs(X + Xoffset, Y + Yoffset);
    D_text(text);
}

void show_label_line(const struct line_pnts *Points, int ltype, LATTR *lattr, const char *text)
{
    double X, Y;

    if ((ltype & GV_POINTS) || Points->n_points == 1)
	/* point/centroid or line/boundary with one coor */
    {
	X = Points->x[0];
	Y = Points->y[0];
    }
    else if (Points->n_points == 2) {	/* line with two coors */
	X = (Points->x[0] + Points->x[1]) / 2;
	Y = (Points->y[0] + Points->y[1]) / 2;
    }
    else {
	int i = Points->n_points / 2;
	X = Points->x[i];
	Y = Points->y[i];
    }

    show_label(&X, &Y, lattr, text);
}
