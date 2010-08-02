/*!
   \file remove_areas.c

   \brief Vector library - clean geometry (remove small areas)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
 */

#include <grass/config.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/*!
   \brief Remove small areas from the map map.

   Centroid of the area and the longest boundary with adjacent area is
   removed.  Map topology must be built GV_BUILD_CENTROIDS.

   \param[in,out] Map vector map
   \param thresh maximum area size for removed areas
   \param[out] Err vector map where removed lines and centroids are written
   \param removed_area  pointer to where total size of removed area is stored or NULL

   \return number of removed areas 
 */
int
Vect_remove_small_areas(struct Map_info *Map, double thresh,
			struct Map_info *Err, double *removed_area)
{
    int area, nareas;
    int nremoved = 0;
    struct ilist *List;
    struct ilist *AList;
    struct line_pnts *Points;
    struct line_cats *Cats;
    double size_removed = 0.0;

    List = Vect_new_list();
    AList = Vect_new_list();
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nareas = Vect_get_num_areas(Map);
    for (area = 1; area <= Vect_get_num_areas(Map); area++) {
	int i, j, centroid, dissolve_neighbour;
	double length, size;

	if (area <= nareas)
	    G_percent(area, nareas, 1);
	G_debug(3, "area = %d", area);
	if (!Vect_area_alive(Map, area))
	    continue;

	size = Vect_get_area_area(Map, area);
	if (size > thresh)
	    continue;
	size_removed += size;

	/* The area is smaller than the limit -> remove */

	/* Remove centroid */
	centroid = Vect_get_area_centroid(Map, area);
	if (centroid > 0) {
	    if (Err) {
		Vect_read_line(Map, Points, Cats, centroid);
		Vect_write_line(Err, GV_CENTROID, Points, Cats);
	    }
	    Vect_delete_line(Map, centroid);
	}

	/* Find the adjacent area with which the longest boundary is shared */

	Vect_get_area_boundaries(Map, area, List);

	/* Create a list of neighbour areas */
	Vect_reset_list(AList);
	for (i = 0; i < List->n_values; i++) {
	    int line, left, right, neighbour;

	    line = List->value[i];

	    if (!Vect_line_alive(Map, abs(line)))	/* Should not happen */
		G_fatal_error(_("Area is composed of dead boundary"));

	    Vect_get_line_areas(Map, abs(line), &left, &right);
	    if (line > 0)
		neighbour = left;
	    else
		neighbour = right;

	    G_debug(4, "  line = %d left = %d right = %d neighbour = %d",
		    line, left, right, neighbour);

	    Vect_list_append(AList, neighbour);	/* this checks for duplicity */
	}
	G_debug(3, "num neighbours = %d", AList->n_values);

	/* Go through the list of neighours and find that with the longest boundary */
	dissolve_neighbour = 0;
	length = -1.0;
	for (i = 0; i < AList->n_values; i++) {
	    int neighbour1;
	    double l = 0.0;

	    neighbour1 = AList->value[i];
	    G_debug(4, "   neighbour1 = %d", neighbour1);

	    for (j = 0; j < List->n_values; j++) {
		int line, left, right, neighbour2;

		line = List->value[j];
		Vect_get_line_areas(Map, abs(line), &left, &right);
		if (line > 0)
		    neighbour2 = left;
		else
		    neighbour2 = right;

		if (neighbour2 == neighbour1) {
		    Vect_read_line(Map, Points, NULL, abs(line));
		    l += Vect_line_length(Points);
		}
	    }
	    if (l > length) {
		length = l;
		dissolve_neighbour = neighbour1;
	    }
	}

	G_debug(3, "dissolve_neighbour = %d", dissolve_neighbour);

	/* Make list of boundaries to be removed */
	Vect_reset_list(AList);
	for (i = 0; i < List->n_values; i++) {
	    int line, left, right, neighbour;

	    line = List->value[i];
	    Vect_get_line_areas(Map, abs(line), &left, &right);
	    if (line > 0)
		neighbour = left;
	    else
		neighbour = right;

	    G_debug(3, "   neighbour = %d", neighbour);

	    if (neighbour == dissolve_neighbour) {
		Vect_list_append(AList, abs(line));
	    }
	}

	/* Remove boundaries */
	for (i = 0; i < AList->n_values; i++) {
	    int line;

	    line = AList->value[i];

	    if (Err) {
		Vect_read_line(Map, Points, Cats, line);
		Vect_write_line(Err, GV_BOUNDARY, Points, Cats);
	    }
	    Vect_delete_line(Map, line);
	}

	nremoved++;
    }

    if (removed_area)
	*removed_area = size_removed;

    G_verbose_message(_("%d areas of total size %g removed"), nremoved,
		size_removed);

    Vect_build_partial(Map, GV_BUILD_BASE);
    Vect_merge_lines(Map, GV_BOUNDARY, NULL, NULL);

    return (nremoved);
}

/* faster version */
int
Vect_remove_small_areas_faster(struct Map_info *Map, double thresh,
			struct Map_info *Err, double *removed_area)
{
    int area, nareas;
    int nremoved = 0;
    struct ilist *List;
    struct ilist *AList;
    struct line_pnts *Points;
    struct line_cats *Cats;
    double size_removed = 0.0;
    int left, right, dissolve_neighbour;

    List = Vect_new_list();
    AList = Vect_new_list();
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nareas = Vect_get_num_areas(Map);
    for (area = 1; area <= Vect_get_num_areas(Map); area++) {
	int i, centroid, remove_boundary;
	double length, size;

	if (area <= nareas)
	    G_percent(area, nareas, 1);
	G_debug(3, "area = %d", area);
	if (!Vect_area_alive(Map, area))
	    continue;

	size = Vect_get_area_area(Map, area);
	if (size > thresh)
	    continue;
	size_removed += size;

	/* The area is smaller than the limit -> remove */

	/* Remove centroid */
	centroid = Vect_get_area_centroid(Map, area);
	if (centroid > 0) {
	    if (Err) {
		Vect_read_line(Map, Points, Cats, centroid);
		Vect_write_line(Err, GV_CENTROID, Points, Cats);
	    }
	    Vect_delete_line(Map, centroid);
	}

	/* Find the adjacent area with which the longest boundary is shared */

	Vect_get_area_boundaries(Map, area, List);

	/* Go through the list of boundaries and find the longest boundary */
	remove_boundary = 0;
	length = -1.0;
	for (i = 0; i < List->n_values; i++) {
	    int line;
	    double l = 0.0;

	    line = List->value[i];
	    G_debug(4, "   line = %d", line);

	    Vect_read_line(Map, Points, NULL, abs(line));
	    l = Vect_line_length(Points);

	    if (l > length) {
		length = l;
		remove_boundary = line;
	    }
	}

	G_debug(3, "remove_boundary = %d", remove_boundary);

	Vect_get_line_areas(Map, abs(remove_boundary), &left, &right);
	dissolve_neighbour = 0;
	if (remove_boundary > 0)
	    dissolve_neighbour = left;
	else
	    dissolve_neighbour = right;

	G_debug(3, "dissolve_neighbour = %d", dissolve_neighbour);
	G_debug(3, "dissolve_neighbour = %d", dissolve_neighbour);
	if (dissolve_neighbour == 0)
	    G_fatal_error("could not find neighbour to dissolve");

	/* Make list of boundaries to be removed */
	Vect_reset_list(AList);
	for (i = 0; i < List->n_values; i++) {
	    int line, neighbour;

	    line = List->value[i];
	    Vect_get_line_areas(Map, abs(line), &left, &right);
	    if (line > 0)
		neighbour = left;
	    else
		neighbour = right;

	    G_debug(3, "   neighbour = %d", neighbour);

	    if (neighbour == dissolve_neighbour) {
		Vect_list_append(AList, abs(line));
	    }
	}

	/* Remove boundaries */
	for (i = 0; i < AList->n_values; i++) {
	    int line;

	    line = AList->value[i];

	    if (Err) {
		Vect_read_line(Map, Points, Cats, line);
		Vect_write_line(Err, GV_BOUNDARY, Points, Cats);
	    }
	    Vect_delete_line(Map, line);
	}

	nremoved++;
    }

    if (removed_area)
	*removed_area = size_removed;

    G_verbose_message(_("%d areas of total size %g removed"), nremoved,
		size_removed);

    Vect_build_partial(Map, GV_BUILD_BASE);
    Vect_merge_lines(Map, GV_BOUNDARY, NULL, NULL);

    return (nremoved);
}

/* much faster version */
int
Vect_remove_small_areas_fastest(struct Map_info *Map, double thresh,
			struct Map_info *Err, double *removed_area)
{
    int area, nareas;
    int nremoved = 0;
    struct ilist *List;
    struct ilist *AList;
    struct ilist *BList;
    struct ilist *NList;
    struct ilist *IList;
    struct line_pnts *Points;
    struct line_cats *Cats;
    double size_removed = 0.0;
    int dissolve_neighbour;
    int line, left, right, neighbour;
    int nisles, nnisles;

    List = Vect_new_list();
    AList = Vect_new_list();
    BList = Vect_new_list();
    NList = Vect_new_list();
    IList = Vect_new_list();
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nareas = Vect_get_num_areas(Map);
    for (area = 1; area <= Vect_get_num_areas(Map); area++) {
	int i, j, centroid;
	double length, l, size;
	int outer_isle = 1, outer_area = -1;

	if (area <= nareas)
	    G_percent(area, nareas, 1);
	G_debug(3, "area = %d", area);
	if (!Vect_area_alive(Map, area))
	    continue;

	size = Vect_get_area_area(Map, area);
	if (size > thresh)
	    continue;
	size_removed += size;

	/* The area is smaller than the limit -> remove */

	/* Remove centroid */
	centroid = Vect_get_area_centroid(Map, area);
	if (centroid > 0) {
	    if (Err) {
		Vect_read_line(Map, Points, Cats, centroid);
		Vect_write_line(Err, GV_CENTROID, Points, Cats);
	    }
	    Vect_delete_line(Map, centroid);
	}

	/* Find the adjacent area with which the longest boundary is shared */

	Vect_get_area_boundaries(Map, area, List);

	/* Create a list of neighbour areas */
	Vect_reset_list(AList);
	for (i = 0; i < List->n_values; i++) {

	    line = List->value[i];

	    if (!Vect_line_alive(Map, abs(line)))	/* Should not happen */
		G_fatal_error(_("Area is composed of dead boundary"));

	    Vect_get_line_areas(Map, abs(line), &left, &right);
	    if (line > 0)
		neighbour = left;
	    else
		neighbour = right;

	    G_debug(4, "  line = %d left = %d right = %d neighbour = %d",
		    line, left, right, neighbour);

	    Vect_list_append(AList, neighbour);	/* this checks for duplicity */
	}
	G_debug(3, "num neighbours = %d", AList->n_values);

	/* Go through the list of neighbours and find the one with the longest boundary */
	dissolve_neighbour = 0;
	length = -1.0;
	for (i = 0; i < AList->n_values; i++) {
	    int neighbour1;

	    l = 0.0;
	    neighbour1 = AList->value[i];
	    G_debug(4, "   neighbour1 = %d", neighbour1);

	    for (j = 0; j < List->n_values; j++) {
		int neighbour2;

		line = List->value[j];
		Vect_get_line_areas(Map, abs(line), &left, &right);
		if (line > 0)
		    neighbour2 = left;
		else
		    neighbour2 = right;

		if (neighbour2 == neighbour1) {
		    Vect_read_line(Map, Points, NULL, abs(line));
		    l += Vect_line_length(Points);
		}
	    }
	    if (l > length) {
		length = l;
		dissolve_neighbour = neighbour1;
	    }
	}

	G_debug(3, "dissolve_neighbour = %d", dissolve_neighbour);

	if (dissolve_neighbour == 0) {
	    G_fatal_error("could not find neighbour to dissolve");
	}

	/* Make list of boundaries to be removed */
	Vect_reset_list(AList);
	Vect_reset_list(BList);
	for (i = 0; i < List->n_values; i++) {

	    line = List->value[i];
	    Vect_get_line_areas(Map, abs(line), &left, &right);
	    if (line > 0)
		neighbour = left;
	    else
		neighbour = right;

	    G_debug(3, "   neighbour = %d", neighbour);

	    if (neighbour == dissolve_neighbour) {
		Vect_list_append(AList, abs(line));
	    }
	    else
		Vect_list_append(BList, line);
		
	    if (neighbour < 0) {
		if (outer_isle > 0)
		    outer_isle = neighbour;
		else if (outer_isle != neighbour)
		    G_fatal_error("Two different isles outside area");
	    }
	}
	G_debug(3, "remove %d of %d boundaries", AList->n_values, List->n_values);

	/* Get isles inside area */
	Vect_reset_list(IList);
	if ((nisles = Vect_get_area_num_isles(Map, area)) > 0) {
	    for (i = 0; i < nisles; i++) {
		Vect_list_append(IList, Vect_get_area_isle(Map, area, i));
	    }
	}

	/* Remove boundaries */
	for (i = 0; i < AList->n_values; i++) {
	    int ret;

	    line = AList->value[i];

	    if (Err) {
		Vect_read_line(Map, Points, Cats, line);
		Vect_write_line(Err, GV_BOUNDARY, Points, Cats);
	    }
	    /* Vect_delete_line(Map, line); */

	    /* delete the line from coor */
	    ret = V1_delete_line_nat(Map, Map->plus.Line[line]->offset);

	    if (ret == -1) {
		G_fatal_error("Could not delete line from coor");
	    }
	}

	/* update topo */
	if (outer_isle < 0) {
	    /* outer area */
	    outer_area = Vect_get_isle_area(Map, -outer_isle);
	}

	if (dissolve_neighbour > 0) {
	    int nareas_old;

	    G_debug(3, "dissolve with neighbour area");

	    /* get neighbour centroid */
	    centroid = Vect_get_area_centroid(Map, dissolve_neighbour);
	    /* get neighbour isles */
	    if ((nnisles = Vect_get_area_num_isles(Map, dissolve_neighbour)) > 0) {
		for (i = 0; i < nnisles; i++) {
		    Vect_list_append(IList, Vect_get_area_isle(Map, dissolve_neighbour, i));
		}
	    }
	    /* get neighbour boundaries */
	    Vect_get_area_boundaries(Map, dissolve_neighbour, NList);

	    /* delete area from topo */
	    dig_del_area(&(Map->plus), area);
	    /* delete neighbour area from topo */
	    dig_del_area(&(Map->plus), dissolve_neighbour);
	    /* delete boundaries from topo */
	    for (i = 0; i < AList->n_values; i++) {
		line = AList->value[i];
		dig_del_line(&(Map->plus), line);
	    }
	    /* rebuild neighbour area from leftover boundaries */
	    nareas_old = Vect_get_num_areas(Map);

	    for (i = 0; i < BList->n_values; i++) {
		line = BList->value[i];
		if (Map->plus.Line[abs(line)]->left == 0 || Map->plus.Line[abs(line)]->right == 0) {
		    int new_isle;

		    new_isle = Vect_build_line_area(Map, abs(line), (line > 0 ? GV_RIGHT : GV_LEFT));
		    if (new_isle > 0) {
			outer_area = new_isle;
			/* reattach centroid */
			Map->plus.Area[outer_area]->centroid = centroid;
			if (centroid > 0)
			    Map->plus.Line[centroid]->left = outer_area;
		    }
		    else if (new_isle < 0) {
			Vect_list_append(IList, -new_isle);
		    }
		}
		/* check */
		if (Map->plus.Line[abs(line)]->left == 0 || Map->plus.Line[abs(line)]->right == 0)
		    G_fatal_error("dissolve with neighbour area: corrupt topology");
	    }
	    for (i = 0; i < NList->n_values; i++) {
		line = NList->value[i];
		if (!Vect_line_alive(Map, abs(line)))
		    continue;

		if (Map->plus.Line[abs(line)]->left == 0 || Map->plus.Line[abs(line)]->right == 0) {
		    int new_isle;

		    new_isle = Vect_build_line_area(Map, abs(line), (line > 0 ? GV_RIGHT : GV_LEFT));
		    if (new_isle > 0) {
			if (outer_area < 0) {
			    outer_area = new_isle;
			    /* reattach centroid */
			    Map->plus.Area[outer_area]->centroid = centroid;
			    if (centroid > 0)
				Map->plus.Line[centroid]->left = outer_area;
			}
			else
			    G_fatal_error("WTF");
		    }
		    else if (new_isle < 0) {
			Vect_list_append(IList, -new_isle);
		    }
		}
		if (Map->plus.Line[abs(line)]->left == 0 || Map->plus.Line[abs(line)]->right == 0)
		    G_fatal_error("dissolve with neighbour area n bounds: corrupt topology");
	    }
	}
	/* dissolve with outer isle */
	else if (dissolve_neighbour < 0) {
	    int nisles_old;

	    G_debug(3, "dissolve with outer isle");

	    if (dissolve_neighbour != outer_isle)
		G_fatal_error("wrong outer isle to dissolve");

	    /* get isle boundaries */
	    Vect_get_isle_boundaries(Map, -dissolve_neighbour, NList);
	    /* delete area from topo */
	    dig_del_area(&(Map->plus), area);
	    /* delete isle from topo */
	    dig_del_isle(&(Map->plus), -dissolve_neighbour);
	    /* delete boundaries from topo */
	    for (i = 0; i < AList->n_values; i++) {
		line = AList->value[i];
		dig_del_line(&(Map->plus), line);
	    }
	    /* rebuild isles from leftover boundaries */
	    for (i = 0; i < BList->n_values; i++) {
		line = BList->value[i];

		if (Map->plus.Line[abs(line)]->left == 0 || Map->plus.Line[abs(line)]->right == 0) {
		    int new_isle;

		    nisles_old = Vect_get_num_islands(Map);
		    new_isle = Vect_build_line_area(Map, abs(line), (line > 0 ? GV_RIGHT : GV_LEFT));
		    if (new_isle > 0 || (new_isle < 0 && nisles_old == Vect_get_num_islands(Map)))
			G_fatal_error("failed to build new isle");

		    if (new_isle < 0) {
			Vect_list_append(IList, -new_isle);
		    }
		}
		/* check */
		if (Map->plus.Line[abs(line)]->left == 0 || Map->plus.Line[abs(line)]->right == 0)
		    G_fatal_error("dissolve with outer isle: corrupt topology");
	    }
	    for (i = 0; i < NList->n_values; i++) {
		line = NList->value[i];
		if (!Vect_line_alive(Map, abs(line)))
		    continue;

		if (Map->plus.Line[abs(line)]->left == 0 || Map->plus.Line[abs(line)]->right == 0) {
		    int new_isle;

		    new_isle = Vect_build_line_area(Map, abs(line), (line > 0 ? GV_RIGHT : GV_LEFT));
		    if (new_isle >= 0)
			G_fatal_error("WTF");
		    else if (new_isle < 0) {
			Vect_list_append(IList, -new_isle);
		    }
		}
		/* check */
		if (Map->plus.Line[abs(line)]->left == 0 || Map->plus.Line[abs(line)]->right == 0)
		    G_fatal_error("dissolve with outer isle n bounds: corrupt topology");
	    }
	}

	/* attach all isles to outer or new area */
	if (outer_area >= 0) {
	    for (i = 0; i < IList->n_values; i++) {
		Map->plus.Isle[IList->value[i]]->area = outer_area;
		if (outer_area > 0)
		    dig_area_add_isle(&(Map->plus), outer_area, IList->value[i]);
	    }
	}

	nremoved++;
    }

    if (removed_area)
	*removed_area = size_removed;

    G_verbose_message(_("%d areas of total size %g removed"), nremoved,
		size_removed);

    Vect_build_partial(Map, GV_BUILD_BASE);
    Vect_merge_lines(Map, GV_BOUNDARY, NULL, NULL);

    return (nremoved);
}
