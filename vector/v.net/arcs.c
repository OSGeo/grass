#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "proto.h"

static int find_node(struct Map_info *, int, int);

static void field2n(struct line_cats *, int);

/**
 * \brief Create network arcs (edge) based on given point vector map (nodes)
 *
 * \param file input file defining arcs
 * \param Points input vector point map
 * \param Out output vector map
 * \param afield arcs layer 
 * \param nfield nodes layer 
 *
 * \return number of new arcs
 */
int create_arcs(FILE * file, struct Map_info *Pnts,
		struct Map_info *Out, int afield, int nfield)
{
    char buff[1024];
    int lcat, fcat, tcat;
    int node1, node2;
    int narcs;

    struct line_pnts *points, *points2;
    struct line_cats *cats;

    points = Vect_new_line_struct();
    points2 = Vect_new_line_struct();
    points = Vect_new_line_struct();
    cats = Vect_new_cats_struct();

    narcs = 0;

    while (G_getl2(buff, sizeof(buff) - 1, file)) {
	if (sscanf(buff, "%d%d%d", &lcat, &fcat, &tcat) != 3)
	    G_fatal_error(_("Error reading file: '%s'"), buff);

	node1 = find_node(Pnts, afield, fcat);
	node2 = find_node(Pnts, afield, tcat);

	if (node1 < 1 || node2 < 1) {
	    G_warning(_("Skipping arc %d"), lcat);
	    continue;
	}

	/* geometry */
	Vect_read_line(Pnts, points, cats, node1);
	field2n(cats, nfield);
	Vect_write_line(Out, GV_POINT, points, cats);
	Vect_read_line(Pnts, points2, cats, node2);
	field2n(cats, nfield);
	Vect_write_line(Out, GV_POINT, points2, cats);
	Vect_append_points(points, points2, GV_FORWARD);

	/* category */
	Vect_reset_cats(cats);
	Vect_cat_set(cats, afield, lcat);
	Vect_write_line(Out, GV_LINE, points, cats);

	narcs++;
    }

    Vect_destroy_line_struct(points);
    Vect_destroy_cats_struct(cats);

    return narcs;
}

int find_node(struct Map_info *Pnts, int field, int cat)
{
    static struct ilist *list;

    if (!list)
	list = Vect_new_list();

    /* find start node */
    Vect_cidx_find_all(Pnts, field, GV_POINT, cat, list);
    if (list->n_values < 1) {
	G_warning(_("No point with category %d found"), cat);
	return 0;
    }
    if (list->n_values > 1) {
	G_warning(_("More points with category %d found"), cat);
	return 0;
    }

    return list->value[0];
}

void field2n(struct line_cats *cats, int nfield)
{
    int n;

    for (n = 0; n < cats->n_cats; n++) {
	cats->field[n] = nfield;
    }
}
