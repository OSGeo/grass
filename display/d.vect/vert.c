#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "local_proto.h"
#include "plot.h"

int display_vert(struct Map_info *Map, int type, LATTR *lattr, double dsize)
{
    int ltype, i;
    double msize;
    struct line_pnts *Points;

    msize = dsize * (D_d_to_u_col(2.0) - D_d_to_u_col(1.0));	/* do it better */
    
    G_debug(1, "display verteces:");
    Points = Vect_new_line_struct();

    D_RGB_color(lattr->color.R, lattr->color.G, lattr->color.B);

    Vect_rewind(Map);
    Vect_set_constraint_type(Map, type);
    while(TRUE) {
        ltype = Vect_read_next_line(Map, Points, NULL);
	switch (ltype) {
	case -1:
	    G_fatal_error(_("Unable to read vector map"));
	case -2:		/* EOF */
	    return 0;
	}

        if (!(ltype & GV_LINES))
            continue;
        
        for (i = 0; i < Points->n_points; i++) {
            D_plot_icon(Points->x[i], Points->y[i], G_ICON_CROSS, 0, msize);
        }
    }
    Vect_remove_constraints(Map);

    Vect_destroy_line_struct(Points);

    return 0;
}
