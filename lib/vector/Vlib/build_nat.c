/*!
   \file lib/vector/Vlib/build_nat.c

   \brief Vector library - Building topology for native format

   (C) 2001-2013 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <grass/glocale.h>
#include <grass/vector.h>

static struct line_pnts *Points;

/*!
   \brief Build topology 

   \param Map vector map
   \param build build level

   \return 1 on success
   \return 0 on error
 */
int Vect_build_nat(struct Map_info *Map, int build)
{
    struct Plus_head *plus;
    int i, s, type, line, counter;
    off_t offset;
    int side, area;
    struct line_cats *Cats;
    struct P_line *Line;
    struct P_area *Area;
    struct bound_box box;
    
    G_debug(3, "Vect_build_nat() build = %d", build);

    plus = &(Map->plus);

    if (build == plus->built)
	return 1;		/* Do nothing */

    /* Check if upgrade or downgrade */
    if (build < plus->built) {
        /* -> downgrade */
	Vect__build_downgrade(Map, build);
        return 1;
    }

    /* -> upgrade */
    if (!Points)
        Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    
    if (plus->built < GV_BUILD_BASE) {
        int c;
        grass_int64 npoints;
        
	/* 
	 *  We shall go through all primitives in coor file and add
	 *  new node for each end point to nodes structure if the node
	 *  with the same coordinates doesn't exist yet.
	 */

	/* register lines, create nodes */
	Vect_rewind(Map);
	G_message(_("Registering primitives..."));
	i = 0;
	npoints = 0;
	while (TRUE) {
	    /* register line */
	    type = Vect_read_next_line(Map, Points, Cats);

	    /* Note: check for dead lines is not needed, because they
               are skipped by V1_read_next_line() */
	    if (type == -1) {
		G_warning(_("Unable to read vector map"));
		return 0;
	    }
	    else if (type == -2) {
		break;
	    }

	    G_progress(++i, 1e4);
	    
	    npoints += Points->n_points;

	    offset = Map->head.last_offset;

	    G_debug(3, "Register line: offset = %lu", (unsigned long)offset);
	    dig_line_box(Points, &box);
	    line = dig_add_line(plus, type, Points, &box, offset);
	    if (line == 1)
		Vect_box_copy(&(plus->box), &box);
	    else
		Vect_box_extend(&(plus->box), &box);

	    /* Add all categories to category index */
	    if (build == GV_BUILD_ALL) {
		for (c = 0; c < Cats->n_cats; c++) {
		    dig_cidx_add_cat(plus, Cats->field[c], Cats->cat[c],
				     line, type);
		}
		if (Cats->n_cats == 0)	/* add field 0, cat 0 */
		    dig_cidx_add_cat(plus, 0, 0, line, type);
	    }
	}
	G_progress(1, 1);

	G_verbose_message(n_("One primitive registered", "%d primitives registered", plus->n_lines), plus->n_lines);
	G_verbose_message(n_("One vertex registered", "%jd vertices registered", npoints), npoints);

	plus->built = GV_BUILD_BASE;
    }

    if (build < GV_BUILD_AREAS)
	return 1;

    if (plus->built < GV_BUILD_AREAS) {
	/* Build areas */
	/* Go through all bundaries and try to build area for both sides */
	if (plus->n_blines > 0) {
	    counter = 1;
	    G_important_message(_("Building areas..."));
	    G_percent(0, plus->n_blines, 1);
	    for (line = 1; line <= plus->n_lines; line++) {

		/* build */
		if (plus->Line[line] == NULL)
		    continue;		/* dead */

		Line = plus->Line[line];
		if (Line->type != GV_BOUNDARY)
		    continue;

		G_percent(counter++, plus->n_blines, 1);

		for (s = 0; s < 2; s++) {
		    if (s == 0)
			side = GV_LEFT;
		    else
			side = GV_RIGHT;

		    G_debug(3, "Build area for line = %d, side = %d", line, side);
		    Vect_build_line_area(Map, line, side);
		}
	    }
	    G_verbose_message(n_("One area built", "%d areas built", plus->n_areas), plus->n_areas);
	    G_verbose_message(n_("One isle built", "%d isles built", plus->n_isles), plus->n_isles);
	}
	plus->built = GV_BUILD_AREAS;
    }

    if (build < GV_BUILD_ATTACH_ISLES)
	return 1;

    /* Attach isles to areas */
    if (plus->built < GV_BUILD_ATTACH_ISLES) {
	if (plus->n_isles > 0) {
	    G_important_message(_("Attaching islands..."));
	    G_percent(0, plus->n_isles, 1);
	    for (i = 1; i <= plus->n_isles; i++) {
		G_percent(i, plus->n_isles, 1);
		Vect_get_isle_box(Map, i, &box);
		Vect_attach_isle(Map, i, &box);
	    }
	}
	plus->built = GV_BUILD_ATTACH_ISLES;
    }

    if (build < GV_BUILD_CENTROIDS)
	return 1;

    /* Attach centroids to areas */
    if (plus->built < GV_BUILD_CENTROIDS) {
	struct P_topo_c *topo;

	if (plus->n_blines > 0) {
	    counter = 1;
	    G_important_message(_("Attaching centroids..."));
	    G_percent(0, plus->n_clines, 1);

	    for (line = 1; line <= plus->n_lines; line++) {

		Line = plus->Line[line];
		if (!Line)
		    continue;	/* dead */

		if (Line->type != GV_CENTROID)
		    continue;

		G_percent(counter++, plus->n_clines, 1);

		Vect_read_line(Map, Points, NULL, line);
		area = Vect_find_area(Map, Points->x[0], Points->y[0]);

		if (area > 0) {
		    G_debug(3, "Centroid (line=%d) in area %d", line, area);

		    Area = plus->Area[area];
		    topo = (struct P_topo_c *)Line->topo;

		    if (Area->centroid == 0) {	/* first */
			Area->centroid = line;
			topo->area = area;
		    }
		    else {		/* duplicate */
			topo->area = -area;
		    }
		}
	    }
	}
	plus->built = GV_BUILD_CENTROIDS;
    }

    /* Add areas to category index */
    /* add message and G_percent() ? 
     * it seems fast enough, no message / precent needed */
    for (i = 1; i <= plus->n_areas; i++) {
	int c;

	if (plus->Area[i] == NULL)
	    continue;

	if (plus->Area[i]->centroid > 0) {
	    Vect_read_line(Map, NULL, Cats, plus->Area[i]->centroid);

	    for (c = 0; c < Cats->n_cats; c++) {
		dig_cidx_add_cat(plus, Cats->field[c], Cats->cat[c], i,
				 GV_AREA);
	    }
	}

	if (plus->Area[i]->centroid == 0 || Cats->n_cats == 0)	/* no centroid or no cats */
	    dig_cidx_add_cat(plus, 0, 0, i, GV_AREA);
    }

    Vect_destroy_cats_struct(Cats);
    
    return 1;
}
