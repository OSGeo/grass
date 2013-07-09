/*!
   \file lib/vector/Vlib/remove_areas.c

   \brief Vector library - clean geometry (remove small areas)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
 */

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/* #define DEGUG_RMAREA */

int Vect_remove_small_areas_nat(struct Map_info *, double,
                                struct Map_info *, double *);

int Vect_remove_small_areas_ext(struct Map_info *, double,
                                struct Map_info *, double *);

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

    if (Map->format == GV_FORMAT_NATIVE)
	return Vect_remove_small_areas_nat(Map, thresh, Err, removed_area);
    else
	return Vect_remove_small_areas_ext(Map, thresh, Err, removed_area);
}

int
Vect_remove_small_areas_ext(struct Map_info *Map, double thresh,
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
    for (area = 1; area <= nareas; area++) {
	int i, j, centroid, dissolve_neighbour;
	double length, size;

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
	nareas = Vect_get_num_areas(Map);
    }

    if (removed_area)
	*removed_area = size_removed;

    G_message(_("%d areas of total size %g removed"), nremoved,
              size_removed);

    return (nremoved);
}


/* much faster version */
int
Vect_remove_small_areas_nat(struct Map_info *Map, double thresh,
			struct Map_info *Err, double *removed_area)
{
    int area, nareas;
    int nremoved = 0;
    struct ilist *List;
    struct ilist *AList;
    struct ilist *BList;
#ifdef DEGUG_RMAREA
    struct ilist *NList;
#endif
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
#ifdef DEGUG_RMAREA
    NList = Vect_new_list();
#endif
    IList = Vect_new_list();
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nareas = Vect_get_num_areas(Map);
    for (area = 1; area <= nareas; area++) {
	int i, j, centroid, ncentroid;
	double length, l, size;
	int outer_area = -1;
	int narea, same_atype = 0;

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

	    ncentroid = 0;
	    if (neighbour > 0) {
		ncentroid = Vect_get_area_centroid(Map, neighbour);
	    }
	    if (neighbour < 0) {
		narea = Vect_get_isle_area(Map, -neighbour);
		if (narea > 0)
		    ncentroid = Vect_get_area_centroid(Map, narea);
	    }
	    if ((centroid != 0) + (ncentroid != 0) != 1)
		same_atype = 1;

	    Vect_list_append(AList, neighbour);	/* this checks for duplicity */
	}
	G_debug(3, "num neighbours = %d", AList->n_values);

	if (AList->n_values == 1)
	    same_atype = 0;

	/* Go through the list of neighbours and find the one with the longest boundary */
	dissolve_neighbour = 0;
	length = -1.0;
	for (i = 0; i < AList->n_values; i++) {
	    int neighbour1;

	    l = 0.0;
	    neighbour1 = AList->value[i];
	    G_debug(4, "   neighbour1 = %d", neighbour1);
	    
	    if (same_atype) {
		ncentroid = 0;
		if (neighbour1 > 0) {
		    ncentroid = Vect_get_area_centroid(Map, neighbour1);
		}
		if (neighbour1 < 0) {
		    narea = Vect_get_isle_area(Map, -neighbour1);
		    if (narea > 0)
			ncentroid = Vect_get_area_centroid(Map, narea);
		}
		if ((centroid != 0) + (ncentroid != 0) == 1)
		    continue;
	    }

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
		G_fatal_error(_("Could not delete line from coor"));
	    }
	}

	/* update topo */
	if (dissolve_neighbour > 0) {

	    G_debug(3, "dissolve with neighbour area");

	    /* get neighbour centroid */
	    centroid = Vect_get_area_centroid(Map, dissolve_neighbour);
	    /* get neighbour isles */
	    if ((nnisles = Vect_get_area_num_isles(Map, dissolve_neighbour)) > 0) {
		for (i = 0; i < nnisles; i++) {
		    Vect_list_append(IList, Vect_get_area_isle(Map, dissolve_neighbour, i));
		}
	    }
#ifdef DEGUG_RMAREA
	    /* get neighbour boundaries */
	    Vect_get_area_boundaries(Map, dissolve_neighbour, NList);
#endif
	    /* delete area from topo */
	    dig_del_area(&(Map->plus), area);
	    /* delete neighbour area from topo */
	    dig_del_area(&(Map->plus), dissolve_neighbour);
	    /* delete boundaries from topo */
	    for (i = 0; i < AList->n_values; i++) {
		struct P_topo_b *topo;
		struct P_node *Node;

		line = AList->value[i];
		topo = (struct P_topo_b *)Map->plus.Line[line]->topo;
		Node = Map->plus.Node[topo->N1];
		dig_del_line(&(Map->plus), line, Node->x, Node->y, Node->z);
	    }
	    /* rebuild neighbour area from leftover boundaries */
	    for (i = 0; i < BList->n_values; i++) {
		struct P_topo_b *topo;

		line = BList->value[i];
		topo = Map->plus.Line[abs(line)]->topo;
		if (topo->left == 0 || topo->right == 0) {
		    int new_isle;

		    new_isle = Vect_build_line_area(Map, abs(line), (line > 0 ? GV_RIGHT : GV_LEFT));
		    if (new_isle > 0) {
			outer_area = new_isle;
			/* reattach centroid */
			Map->plus.Area[outer_area]->centroid = centroid;
			if (centroid > 0) {
			    struct P_topo_c *ctopo = Map->plus.Line[centroid]->topo;

			    ctopo->area = outer_area;
			}
		    }
		    else {
			/* should not happen */
			G_warning(_("Failed to build new area"));
		    }
		}
		/* check */
		if (topo->left == 0 || topo->right == 0)
		    G_warning(_("Dissolve with neighbour area: corrupt topology"));
	    }
#ifdef DEGUG_RMAREA
	    for (i = 0; i < NList->n_values; i++) {
		struct P_topo_b *topo;

		line = NList->value[i];
		if (!Vect_line_alive(Map, abs(line)))
		    continue;

		topo = Map->plus.Line[abs(line)]->topo;
		if (topo->left == 0 || topo->right == 0) {
		    int new_isle;

		    G_warning(_("Neighbor boundaries should be complete"));

		    new_isle = Vect_build_line_area(Map, abs(line), (line > 0 ? GV_RIGHT : GV_LEFT));
		    if (new_isle > 0) {
			outer_area = new_isle;
			/* reattach centroid */
			Map->plus.Area[outer_area]->centroid = centroid;
			if (centroid > 0) {
			    struct P_topo_c *ctopo = Map->plus.Line[centroid]->topo;

			    ctopo->area = outer_area;
			}
		    }
		    else {
			/* should not happen */
			G_warning(_("Failed to build new area"));
		    }
		}
		if (topo->left == 0 || topo->right == 0)
		    G_warning(_("Dissolve with neighbour area: corrupt topology"));
	    }
#endif
	}
	/* dissolve with outer isle */
	else if (dissolve_neighbour < 0) {

	    G_debug(3, "dissolve with outer isle");

	    outer_area = Vect_get_isle_area(Map, -dissolve_neighbour);

#ifdef DEGUG_RMAREA
	    /* get isle boundaries */
	    Vect_get_isle_boundaries(Map, -dissolve_neighbour, NList);
#endif
	    /* delete area from topo */
	    dig_del_area(&(Map->plus), area);
	    /* delete isle from topo */
	    dig_del_isle(&(Map->plus), -dissolve_neighbour);
	    /* delete boundaries from topo */
	    for (i = 0; i < AList->n_values; i++) {
		struct P_topo_b *topo;
		struct P_node *Node;

		line = AList->value[i];
		topo = (struct P_topo_b *)Map->plus.Line[line]->topo;
		Node = Map->plus.Node[topo->N1];
		dig_del_line(&(Map->plus), line, Node->x, Node->y, Node->z);
	    }
	    /* rebuild isles from leftover boundaries */
	    for (i = 0; i < BList->n_values; i++) {
		struct P_topo_b *topo;

		line = BList->value[i];
		topo = Map->plus.Line[abs(line)]->topo;

		if (topo->left == 0 || topo->right == 0) {
		    int new_isle;

		    new_isle = Vect_build_line_area(Map, abs(line), (line > 0 ? GV_RIGHT : GV_LEFT));
		    if (new_isle < 0) {
			Vect_list_append(IList, -new_isle);
		    }
		    else {
			/* should not happen */
			G_warning(_("Failed to build new isle"));
		    }
		}
		/* check */
		if (topo->left == 0 || topo->right == 0)
		    G_warning(_("Dissolve with outer isle: corrupt topology"));
	    }
#ifdef DEGUG_RMAREA
	    for (i = 0; i < NList->n_values; i++) {
		struct P_topo_b *topo;

		line = NList->value[i];
		if (!Vect_line_alive(Map, abs(line)))
		    continue;

		topo = Map->plus.Line[abs(line)]->topo;
		if (topo->left == 0 || topo->right == 0) {
		    int new_isle;

		    G_warning(_("Neighbor boundaries should be complete"));

		    new_isle = Vect_build_line_area(Map, abs(line), (line > 0 ? GV_RIGHT : GV_LEFT));
		    if (new_isle < 0) {
			Vect_list_append(IList, -new_isle);
		    else {
			/* should not happen */
			G_warning(_("Failed to build new isle"));
		    }
		}
		/* check */
		if (topo->left == 0 || topo->right == 0)
		    G_warning(_("Dissolve with outer isle: corrupt topology"));
	    }
#endif
	}

	if (dissolve_neighbour > 0 && outer_area <= 0) {
	    G_fatal_error(_("Area merging failed"));
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
	nareas = Vect_get_num_areas(Map);
    }

    if (removed_area)
	*removed_area = size_removed;

    G_message(_("%d areas of total size %g removed"), nremoved,
              size_removed);

    Vect_destroy_list(List);
    Vect_destroy_list(AList);
    Vect_destroy_list(BList);
#ifdef DEGUG_RMAREA
    Vect_destroy_list(NList);
#endif
    Vect_destroy_list(IList);
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return (nremoved);
}
