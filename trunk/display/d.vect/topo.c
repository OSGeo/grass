#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "local_proto.h"
#include "plot.h"

int display_topo(struct Map_info *Map, int type, LATTR *lattr, double dsize)
{
    int ltype, num, el;
    double msize;
    struct line_pnts *Points;
    struct line_cats *Cats;
    char text[50];
    LATTR lattr2 = *lattr;

    if (Vect_level(Map) < 2) {
	G_warning(_("Unable to display topology, not available."
		    "Please try to rebuild topology using "
		    "v.build or v.build.all."));
	return 1;
    }
    
    msize = dsize * (D_d_to_u_col(2.0) - D_d_to_u_col(1.0));	/* do it better */
    
    lattr2.xref = lattr->xref == LRIGHT ? LLEFT : LRIGHT;

    G_debug(1, "display topo:");
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    D_RGB_color(lattr->color.R, lattr->color.G, lattr->color.B);
    D_text_size(lattr->size, lattr->size);
    if (lattr->font)
	D_font(lattr->font);
    if (lattr->enc)
	D_encoding(lattr->enc);

    Vect_rewind(Map);

    num = Vect_get_num_lines(Map);
    G_debug(1, "n_lines = %d", num);

    /* Lines */
    for (el = 1; el <= num; el++) {
	if (!Vect_line_alive(Map, el))
	    continue;
	ltype = Vect_read_line(Map, Points, Cats, el);
	G_debug(3, "ltype = %d", ltype);
	switch (ltype) {
	case -1:
	    G_fatal_error(_("Unable to read vector map"));
	case -2:		/* EOF */
	    return 0;
	}

	if (!(type & ltype))
	    continue;		/* used for both lines and labels */

	sprintf(text, "%d", el);
	show_label_line(Points, ltype, lattr, text);
    }

    num = Vect_get_num_nodes(Map);
    G_debug(1, "n_nodes = %d", num);

    /* Nodes */
    for (el = 1; el <= num; el++) {
	double X, Y;
	if (!Vect_node_alive(Map, el))
	    continue;
	Vect_get_node_coor(Map, el, &X, &Y, NULL);
	G_debug(3, "node = %d", el);
	sprintf(text, "n%d", el);

	show_label(&X, &Y, &lattr2, text);

	D_plot_icon(X, Y, G_ICON_BOX, 0, msize);
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return 0;
}
