/*!
   \file lib/vector/Vlib/write.c

   \brief Vector library - write vector features

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2010 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
   \author Updated by Martin Landa <landa.martin gmail.com> (restore lines, OGR support)
 */

#include <grass/config.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/vector.h>

static off_t write_dummy()
{
    G_warning("Vect_write_line() %s",
	      _("for this format/level not supported"));
    return -1;
}
static int rewrite_dummy()
{
    G_warning("Vect_rewrite_line() %s",
	      _("for this format/level not supported"));
    return -1;
}
static int delete_dummy()
{
    G_warning("Vect_delete_line() %s",
	      _("for this format/level not supported"));
    return -1;
}

static int restore_dummy()
{
    G_warning("Vect_restore_line() %s",
	      _("for this format/level not supported"));
    return -1;
}

#ifndef HAVE_OGR
static int format()
{
    G_fatal_error(_("Requested format is not compiled in this version"));
    return 0;
}

static off_t format_l()
{
    G_fatal_error(_("Requested format is not compiled in this version"));
    return 0;
}

#endif

static off_t (*Write_line_array[][3]) () = {
    {
    write_dummy, V1_write_line_nat, V2_write_line_nat}
#ifdef HAVE_OGR
    , {
    write_dummy, V1_write_line_ogr, V2_write_line_ogr}
    , {
    write_dummy, V1_write_line_ogr, V2_write_line_ogr}
#else
    , {
    write_dummy, format_l, format_l}
    , {
    write_dummy, format_l, format_l}
#endif
};

static int (*Vect_rewrite_line_array[][3]) () = {
    {
    rewrite_dummy, V1_rewrite_line_nat, V2_rewrite_line_nat}
#ifdef HAVE_OGR
    , {
    rewrite_dummy, V1_rewrite_line_ogr, V2_rewrite_line_ogr}
    , {
    rewrite_dummy, V1_rewrite_line_ogr, V2_rewrite_line_ogr}
#else
    , {
    rewrite_dummy, format, format}
    , {
    rewrite_dummy, format, format}
#endif
};

static int (*Vect_delete_line_array[][3]) () = {
    {
    delete_dummy, V1_delete_line_nat, V2_delete_line_nat}
#ifdef HAVE_OGR
    , {
    delete_dummy, V1_delete_line_ogr, V2_delete_line_ogr}
    , {
    delete_dummy, V1_delete_line_ogr, V2_delete_line_ogr}
#else
    , {
    delete_dummy, format, format}
    , {
    delete_dummy, format, format}
#endif
};

static int (*Vect_restore_line_array[][3]) () = {
    {
    restore_dummy, restore_dummy, V2_restore_line_nat}
#ifdef HAVE_OGR
    , {
    restore_dummy, restore_dummy, restore_dummy}
    , {
    restore_dummy, restore_dummy, restore_dummy}
#else
    , {
    restore_dummy, format, format}
    , {
    restore_dummy, format, format}
#endif
};

/*! 
  \brief Deletes area (i.e. centroid) categories from category
  index (internal use only)

  \param Map pointer to Map_info structure
  \param area area id
*/
void Vect__delete_area_cats_from_cidx(struct Map_info *Map, int area)
{
    int i;
    struct P_area *Area;
    static struct line_cats *Cats = NULL;

    G_debug(3, "Vect__delete_area_cats_from_cidx() area = %d", area);

    Area = Map->plus.Area[area];
    if (!Area)
	G_fatal_error(_("%s: Area %d does not exist"),
		      "delete_area_cats_from_cidx()", area);
    
    if (Area->centroid == 0) /* no centroid found */
	return;

    if (!Cats)
	Cats = Vect_new_cats_struct();

    V2_read_line_nat(Map, NULL, Cats, Area->centroid);

    for (i = 0; i < Cats->n_cats; i++) {
	dig_cidx_del_cat(&(Map->plus), Cats->field[i], Cats->cat[i], area,
			 GV_AREA);
    }
}

/*! 
  \brief Adds area (i.e. centroid) categories from category index
  (internal use only)

  \param Map pointer to Map_info structure
  \param area area id
*/
void Vect__add_area_cats_to_cidx(struct Map_info *Map, int area)
{
    int i;
    struct P_area *Area;
    static struct line_cats *Cats = NULL;

    G_debug(3, "Vect__add_area_cats_to_cidx() area = %d", area);

    Area = Map->plus.Area[area];
    if (!Area)
	G_fatal_error(_("%s: Area %d does not exist"),
		      "add_area_cats_to_cidx():", area);

    if (Area->centroid == 0) /* no centroid found */
	return;

    if (!Cats)
	Cats = Vect_new_cats_struct();

    V2_read_line_nat(Map, NULL, Cats, Area->centroid);

    for (i = 0; i < Cats->n_cats; i++) {
	dig_cidx_add_cat_sorted(&(Map->plus), Cats->field[i], Cats->cat[i],
				area, GV_AREA);
    }
}

/*!
  \brief Deletes feature (topology level) -- internal use only
  
  \param pointer to Map_info structure
  \param line feature id
  \param fn function to delete feature (native or ogr)
  
  \return 0 on success
  \return -1 on error
*/
int V2__delete_line(struct Map_info *Map, int line, int (*fn_delete) (struct Map_info *, off_t))
{
    int ret, i, side, type, first, next_line, area;
    struct P_line *Line;
    struct P_area *Area;
    struct Plus_head *plus;
    struct bound_box box, abox;
    int adjacent[4], n_adjacent;
    static struct line_cats *Cats = NULL;

    G_debug(3, "V2__delete_line(), line = %d", line);

    type = first = n_adjacent = 0;
    Line = NULL;
    plus = &(Map->plus);

    if (plus->built >= GV_BUILD_BASE) {
	Line = Map->plus.Line[line];

	if (Line == NULL)
	    G_fatal_error(_("Attempt to delete dead feature"));
	type = Line->type;
    }

    if (!Cats) {
	Cats = Vect_new_cats_struct();
    }

    /* Update category index */
    if (plus->update_cidx) {
	type = V2_read_line_nat(Map, NULL, Cats, line);

	for (i = 0; i < Cats->n_cats; i++) {
	    dig_cidx_del_cat(plus, Cats->field[i], Cats->cat[i], line, type);
	}
    }

    /* delete the line from coor */
    ret = fn_delete(Map, Line->offset);

    if (ret == -1) {
	return ret;
    }

    /* Update topology */
    if (plus->built >= GV_BUILD_AREAS && type == GV_BOUNDARY) {
	/* Store adjacent boundaries at nodes (will be used to rebuild area/isle) */
	/* Adjacent are stored: > 0 - we want right side; < 0 - we want left side */
	n_adjacent = 0;

	next_line = dig_angle_next_line(plus, line, GV_RIGHT, GV_BOUNDARY);
	if (next_line != 0 && abs(next_line) != line) {
	    /* N1, to the right -> we want the right side for > 0  and left for < 0 */
	    adjacent[n_adjacent] = next_line;
	    n_adjacent++;
	}
	next_line = dig_angle_next_line(plus, line, GV_LEFT, GV_BOUNDARY);
	if (next_line != 0 && abs(next_line) != line) {
	    /* N1, to the left -> we want the left side for > 0  and right for < 0 */
	    adjacent[n_adjacent] = -next_line;
	    n_adjacent++;
	}
	next_line = dig_angle_next_line(plus, -line, GV_RIGHT, GV_BOUNDARY);
	if (next_line != 0 && abs(next_line) != line) {
	    /* N2, to the right -> we want the right side for > 0  and left for < 0 */
	    adjacent[n_adjacent] = next_line;
	    n_adjacent++;
	}
	next_line = dig_angle_next_line(plus, -line, GV_LEFT, GV_BOUNDARY);
	if (next_line != 0 && abs(next_line) != line) {
	    /* N2, to the left -> we want the left side for > 0  and right for < 0 */
	    adjacent[n_adjacent] = -next_line;
	    n_adjacent++;
	}

	/* Delete area(s) and islands this line forms */
	first = 1;
	if (Line->left > 0) {	/* delete area */
	    Vect_get_area_box(Map, Line->left, &box);
	    if (first) {
		Vect_box_copy(&abox, &box);
		first = 0;
	    }
	    else
		Vect_box_extend(&abox, &box);

	    if (plus->update_cidx) {
		Vect__delete_area_cats_from_cidx(Map, Line->left);
	    }
	    dig_del_area(plus, Line->left);
	}
	else if (Line->left < 0) {	/* delete isle */
	    dig_del_isle(plus, -Line->left);
	}
	if (Line->right > 0) {	/* delete area */
	    Vect_get_area_box(Map, Line->right, &box);
	    if (first) {
		Vect_box_copy(&abox, &box);
		first = 0;
	    }
	    else
		Vect_box_extend(&abox, &box);

	    if (plus->update_cidx) {
		Vect__delete_area_cats_from_cidx(Map, Line->right);
	    }
	    dig_del_area(plus, Line->right);
	}
	else if (Line->right < 0) {	/* delete isle */
	    dig_del_isle(plus, -Line->right);
	}
    }

    /* Delete reference from area */
    if (plus->built >= GV_BUILD_CENTROIDS && type == GV_CENTROID) {
	if (Line->left > 0) {
	    G_debug(3, "Remove centroid %d from area %d", line, Line->left);
	    if (plus->update_cidx) {
		Vect__delete_area_cats_from_cidx(Map, Line->left);
	    }
	    Area = Map->plus.Area[Line->left];
	    Area->centroid = 0;
	}
    }

    /* delete the line from topo */
    dig_del_line(plus, line);

    /* Rebuild areas/isles and attach centroids and isles */
    if (plus->built >= GV_BUILD_AREAS && type == GV_BOUNDARY) {
	int *new_areas, nnew_areas;

	nnew_areas = 0;
	new_areas = (int *)G_malloc(2 * n_adjacent * sizeof(int));
	/* Rebuild areas/isles */
	for (i = 0; i < n_adjacent; i++) {
	    if (adjacent[i] > 0)
		side = GV_RIGHT;
	    else
		side = GV_LEFT;

	    G_debug(3, "Build area for line = %d, side = %d", adjacent[i],
		    side);

	    area = Vect_build_line_area(Map, abs(adjacent[i]), side);
	    if (area > 0) {	/* area */
		Vect_get_area_box(Map, area, &box);
		if (first) {
		    Vect_box_copy(&abox, &box);
		    first = 0;
		}
		else
		    Vect_box_extend(&abox, &box);

		new_areas[nnew_areas] = area;
		nnew_areas++;
	    }
	    else if (area < 0) {
		/* isle -> must be attached -> add to abox */
		Vect_get_isle_box(Map, -area, &box);
		if (first) {
		    Vect_box_copy(&abox, &box);
		    first = 0;
		}
		else
		    Vect_box_extend(&abox, &box);
	    }
	}
	/* Reattach all centroids/isles in deleted areas + new area.
	 *  Because isles are selected by box it covers also possible new isle created above */
	if (!first) {		/* i.e. old area/isle was deleted or new one created */
	    /* Reattache isles */
	    if (plus->built >= GV_BUILD_ATTACH_ISLES)
		Vect_attach_isles(Map, &abox);

	    /* Reattach centroids */
	    if (plus->built >= GV_BUILD_CENTROIDS)
		Vect_attach_centroids(Map, &abox);
	}

	if (plus->update_cidx) {
	    for (i = 0; i < nnew_areas; i++) {
		Vect__add_area_cats_to_cidx(Map, new_areas[i]);
	    }
	}
    }

    G_debug(3, "updated lines : %d , updated nodes : %d", plus->n_uplines,
	    plus->n_upnodes);
    return ret;
}

/*!
  \brief Add line to topo file

  Update areas. Areas are modified if: 
   
  1) first or/and last point are existing nodes ->
   - drop areas/islands whose boundaries are neighbour to this boundary at these nodes
   - try build areas and islands for this boundary and neighbour boundaries going through these nodes

   Question: may be by adding line created new area/isle which doesn't go through nodes of this line

   <pre>
             old         new line 
         +----+----+                    +----+----+                 +----+----+ 
         | A1 | A2 |  +      /      ->  | A1 |   /|   or +   \   -> | A1 | A2 | \
         |    |    |                    |    |    |                 |    |    |
         +----+----+                    +----+----+                 +----+----+
           I1   I1                        I1   I1                      
   </pre>        
 
   - reattache all centroids/isles inside new area(s)
   - attach new isle to area outside

  2) line is closed ring (node at the end is new, so it is not case above)
    - build new area/isle
    - check if it is island or contains island(s)
    - re-attach all centroids/isles inside new area(s)
    - attach new isle to area outside
    
    Note that 1) and 2) is done by the same code.
*/
void Vect__add_line_to_topo(struct Map_info *Map, int line,
			    const struct line_pnts *points, const struct line_cats *cats)
{
    int first, s, n, i;
    int type, node, next_line, area, side, sel_area, new_area[2];

    struct Plus_head *plus;
    struct P_line *Line, *NLine;
    struct P_node *Node;
    struct P_area *Area;
    
    struct bound_box box, abox;
    
    plus = &(Map->plus);
    Line = plus->Line[line];
    type = Line->type;

    if (plus->built >= GV_BUILD_AREAS) {
	if (type == GV_BOUNDARY) {
	    /* Delete neighbour areas/isles */
	    first = 1;
	    for (s = 1; s < 3; s++) {	/* for each node */
		if (s == 1)
		    node = Line->N1;	/* Node 1 */
		else
		    node = Line->N2;
		G_debug(3,
			"  delete neighbour areas/iseles: side = %d node = %d",
			s, node);
		Node = plus->Node[node];
		n = 0;
		for (i = 0; i < Node->n_lines; i++) {
		    NLine = plus->Line[abs(Node->lines[i])];
		    if (NLine->type == GV_BOUNDARY)
			n++;
		}

		G_debug(3, "  number of boundaries at node = %d", n);
		if (n > 2) {	/* more than 2 boundaries at node ( >= 2 old + 1 new ) */
		    /* Line above (to the right), it is enough to check to the right, because if area/isle
		     *  exists it is the same to the left */
		    if (s == 1)
			next_line =
			    dig_angle_next_line(plus, line, GV_RIGHT,
						GV_BOUNDARY);
		    else
			next_line =
			    dig_angle_next_line(plus, -line, GV_RIGHT,
						GV_BOUNDARY);

		    if (next_line != 0) {	/* there is a boundary to the right */
			NLine = plus->Line[abs(next_line)];
			if (next_line > 0)	/* the boundary is connected by 1. node */
			    area = NLine->right;	/* we are interested just in this side (close to our line) */
			else if (next_line < 0)	/* the boundary is connected by 2. node */
			    area = NLine->left;

			G_debug(3, "  next_line = %d area = %d", next_line,
				area);
			if (area > 0) {	/* is area */
			    Vect_get_area_box(Map, area, &box);
			    if (first) {
				Vect_box_copy(&abox, &box);
				first = 0;
			    }
			    else
				Vect_box_extend(&abox, &box);

			    if (plus->update_cidx) {
				Vect__delete_area_cats_from_cidx(Map, area);
			    }
			    dig_del_area(plus, area);
			}
			else if (area < 0) {	/* is isle */
			    dig_del_isle(plus, -area);
			}
		    }
		}
	    }
	    /* Build new areas/isles. Thas true that we deleted also adjacent areas/isles, but if
	     *  they form new one our boundary must participate, so we need to build areas/isles
	     *  just for our boundary */
	    for (s = 1; s < 3; s++) {
		if (s == 1)
		    side = GV_LEFT;
		else
		    side = GV_RIGHT;
		G_debug(3, "  build area/isle on side = %d", side);

		G_debug(3, "Build area for line = %d, side = %d", line, side);
		area = Vect_build_line_area(Map, line, side);
		G_debug(3, "Build area for line = %d, side = %d", line, side);
		if (area > 0) {	/* area */
		    Vect_get_area_box(Map, area, &box);
		    if (first) {
			Vect_box_copy(&abox, &box);
			first = 0;
		    }
		    else
			Vect_box_extend(&abox, &box);
		}
		else if (area < 0) {
		    /* isle -> must be attached -> add to abox */
		    Vect_get_isle_box(Map, -area, &box);
		    if (first) {
			Vect_box_copy(&abox, &box);
			first = 0;
		    }
		    else
			Vect_box_extend(&abox, &box);
		}
		new_area[s - 1] = area;
	    }
	    /* Reattach all centroids/isles in deleted areas + new area.
	     *  Because isles are selected by box it covers also possible new isle created above */
	    if (!first) {	/* i.e. old area/isle was deleted or new one created */
		/* Reattache isles */
		if (plus->built >= GV_BUILD_ATTACH_ISLES)
		    Vect_attach_isles(Map, &abox);

		/* Reattach centroids */
		if (plus->built >= GV_BUILD_CENTROIDS)
		    Vect_attach_centroids(Map, &abox);
	    }
	    /* Add to category index */
	    if (plus->update_cidx) {
		for (s = 1; s < 3; s++) {
		    if (new_area[s - 1] > 0) {
			Vect__add_area_cats_to_cidx(Map, new_area[s - 1]);
		    }
		}
	    }
	}
    }

    /* Attach centroid */
    if (plus->built >= GV_BUILD_CENTROIDS) {
	if (type == GV_CENTROID) {
	    sel_area = Vect_find_area(Map, points->x[0], points->y[0]);
	    G_debug(3, "  new centroid %d is in area %d", line, sel_area);
	    if (sel_area > 0) {
		Area = plus->Area[sel_area];
		Line = plus->Line[line];
		if (Area->centroid == 0) {	/* first centroid */
		    G_debug(3, "  first centroid -> attach to area");
		    Area->centroid = line;
		    Line->left = sel_area;
		    if (plus->update_cidx) {
			Vect__add_area_cats_to_cidx(Map, sel_area);
		    }
		}
		else {		/* duplicate centroid */
		    G_debug(3,
			    "  duplicate centroid -> do not attach to area");
		    Line->left = -sel_area;
		}
	    }
	}
    }

    /* Add cetegory index */
    for (i = 0; i < cats->n_cats; i++) {
	dig_cidx_add_cat_sorted(plus, cats->field[i], cats->cat[i], line,
				type);
    }

    return;
}

/*!
  \brief Writes feature to 'coor' file (topology level) - internal use only
  
  \param Map pointer to Map_info structure
  \param type feature type
  \param points feature geometry
  \param cats feature categories
  
  \return new feature id
  \return -1 on error
*/
off_t V2__write_line(struct Map_info *Map,
		     int type, const struct line_pnts *points, const struct line_cats *cats,
		     off_t (*write_fn) (struct Map_info *, int, const struct line_pnts *,
					const struct line_cats *))
{
    int line;
    off_t offset;
    struct Plus_head *plus;
    struct bound_box box;

    line = 0;
    
    G_debug(3, "V2__write_line()");
    offset = write_fn(Map, type, points, cats);
    if (offset < 0)
	return -1;

    /* Update topology */
    plus = &(Map->plus);
    /* Add line */
    if (plus->built >= GV_BUILD_BASE) {
	line = dig_add_line(plus, type, points, offset);
	G_debug(3, "  line added to topo with id = %d", line);
	dig_line_box(points, &box);
	dig_line_set_box(plus, line, &box);
	if (line == 1)
	    Vect_box_copy(&(plus->box), &box);
	else
	    Vect_box_extend(&(plus->box), &box);
    }

    Vect__add_line_to_topo(Map,
			   line, points, cats);

    G_debug(3, "updated lines : %d , updated nodes : %d", plus->n_uplines,
	    plus->n_upnodes);

    /* returns int line, but is defined as off_t for compatibility with
     * Write_line_array in write.c */
    
    return line;
}

/*!
   \brief Writes new feature to the end of file

   The function calls G_fatal_error() on error.

   \param Map pointer to vector map
   \param type feature type
   \param points feature geometry
   \param cats feature categories

   \return new feature id (level 2)
   \return offset into file where the feature starts (level 1)
 */
off_t
Vect_write_line(struct Map_info *Map,
		int type, const struct line_pnts *points, const struct line_cats *cats)
{
    off_t offset;

    G_debug(3, "Vect_write_line(): name = %s, format = %d, level = %d",
	    Map->name, Map->format, Map->level);

    if (!VECT_OPEN(Map))
	G_fatal_error(_("Unable to write feature, vector map is not opened"));

    dig_line_reset_updated(&(Map->plus));
    dig_node_reset_updated(&(Map->plus));
    if (!(Map->plus.update_cidx)) {
	Map->plus.cidx_up_to_date = 0;
    }

    offset =
	(*Write_line_array[Map->format][Map->level]) (Map, type, points,
						      cats);

    if (offset == -1)
	G_fatal_error(_("Unable to write feature (negative offset)"));

    /* NOTE: returns new line id on level 2 and file offset on level 1 */
    return offset;
}


/*!
   \brief Rewrites feature info at the given offset.

   The number of points or cats or type may change. If necessary, the
   old feature is deleted and new is written.

   This function calls G_fatal_error() on error.

   \param Map pointer to vector map
   \param line feature id
   \param type feature type
   \param points feature geometry
   \param cats feature categories

   \return new feature id
   \return -1 on error
 */
int
Vect_rewrite_line(struct Map_info *Map,
		  int line,
		  int type, const struct line_pnts *points, const struct line_cats *cats)
{
    long ret;

    G_debug(3, "Vect_rewrite_line(): name = %s, line = %d", Map->name, line);

    if (!VECT_OPEN(Map))
	G_fatal_error(_("Unable to rewrite feature, vector map is not opened"));

    dig_line_reset_updated(&(Map->plus));
    dig_node_reset_updated(&(Map->plus));
    if (!(Map->plus.update_cidx)) {
	Map->plus.cidx_up_to_date = 0;
    }

    ret =
	(*Vect_rewrite_line_array[Map->format][Map->level]) (Map, line, type,
							     points, cats);

    if (ret == -1)
	G_fatal_error(_("Unable to rewrite feature %d"), line);

    return ret;
}

/*!
   \brief Delete feature

   Vector map must be opened on topo level 2.

   This function calls G_fatal_error() on error.

   \param Map pointer to vector map
   \param line feature id

   \return 0 on success
   \return -1 on error
 */
int Vect_delete_line(struct Map_info *Map, int line)
{
    int ret;

    G_debug(3, "Vect_delete_line(): name = %s, line = %d", Map->name, line);

    if (Map->level < 2) {
	G_fatal_error(_("Unable to delete feature %d, "
			"vector map <%s> is not opened on topology level"),
		      line, Map->name);
    }

    if (Map->mode != GV_MODE_RW && Map->mode != GV_MODE_WRITE) {
	G_fatal_error(_("Unable to delete feature %d, "
			"vector map <%s> is not opened in 'write' mode"),
		      line, Map->name);
    }

    dig_line_reset_updated(&(Map->plus));
    dig_node_reset_updated(&(Map->plus));
    if (!(Map->plus.update_cidx)) {
	Map->plus.cidx_up_to_date = 0;
    }

    ret = (*Vect_delete_line_array[Map->format][Map->level]) (Map, line);

    if (ret == -1)
	G_fatal_error(_("Unable to delete feature id %d from vector map <%s>"),
		      line, Vect_get_full_name(Map));

    return ret;
}

/*!
   \brief Restore previously deleted feature

   Vector map must be opened on topo level 2.

   This function calls G_fatal_error() on error.

   \param Map pointer to vector map
   \param line feature id to be deleted

   \return 0 on success
   \return -1 on error
 */
int Vect_restore_line(struct Map_info *Map, int line, off_t offset)
{
    int ret;

    G_debug(3, "Vect_restore_line(): name = %s, line = %d", Map->name, line);

    if (Map->level < 2) {
	G_fatal_error(_("Unable to restore feature %d, "
			"vector map <%s> is not opened on topology level"),
		      line, Map->name);
    }

    if (Map->mode != GV_MODE_RW && Map->mode != GV_MODE_WRITE) {
	G_fatal_error(_("Unable to restore feature %d, "
			"vector map <%s> is not opened in 'write' mode"),
		      line, Map->name);
    }

    dig_line_reset_updated(&(Map->plus));
    dig_node_reset_updated(&(Map->plus));
    if (!(Map->plus.update_cidx)) {
	Map->plus.cidx_up_to_date = 0;
    }

    ret = (*Vect_restore_line_array[Map->format][Map->level]) (Map, line, offset);

    if (ret == -1)
	G_fatal_error(_("Unable to restore feature %d from vector map <%s>"),
		      line, Map->name);
    
    return ret;
}
