/*!
   \file lib/vector/Vlib/build.c

   \brief Vector library - Building topology

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2010, 2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>

#include <grass/vector.h>
#include <grass/glocale.h>

#define SEP "-----------------------------------\n"

#if !defined HAVE_OGR || !defined HAVE_POSTGRES
static int format()
{
    G_fatal_error(_("Requested format is not compiled in this version"));
    return 0;
}
#endif

static int (*Build_array[]) () = {
    Vect_build_nat
#ifdef HAVE_OGR
    , Vect_build_ogr
    , Vect_build_ogr
#else
    , format
    , format
#endif
#ifdef HAVE_POSTGRES
    , Vect_build_pg
#else
    , format
#endif
};

/*!
   \brief Build area on given side of line (GV_LEFT or GV_RIGHT)

   \param Map pointer to Map_info structure
   \param iline line id
   \param side side (GV_LEFT or GV_RIGHT)

   \return > 0 area id
   \return < 0 isle id
   \return 0 not created (may also already exist)
 */
int Vect_build_line_area(struct Map_info *Map, int iline, int side)
{
    int j, area, isle, n_lines, line, direction;
    static int first = TRUE;
    struct Plus_head *plus;
    struct P_line *BLine;
    static struct line_pnts *Points, *APoints;
    struct bound_box box;
    plus_t *lines;
    double area_size;

    plus = &(Map->plus);

    G_debug(3, "Vect_build_line_area() line = %d, side = %d", iline, side);

    if (first) {
	Points = Vect_new_line_struct();
	APoints = Vect_new_line_struct();
	first = FALSE;
    }

    area = dig_line_get_area(plus, iline, side);
    if (area != 0) {
        G_debug(3, "  area/isle = %d -> skip", area);
        return 0;
    }
    
    n_lines = dig_build_area_with_line(plus, iline, side, &lines);
    G_debug(3, "  n_lines = %d", n_lines);
    if (n_lines < 1) {
	return 0;
    }				/* area was not built */

    /* Area or island ? */
    Vect_reset_line(APoints);
    for (j = 0; j < n_lines; j++) {
	line = abs(lines[j]);
	BLine = plus->Line[line];
	G_debug(3, "  line[%d] = %d, offset = %lu", j, line,
		(unsigned long) BLine->offset);
	Vect_read_line(Map, Points, NULL, line);
	if (lines[j] > 0)
	    direction = GV_FORWARD;
	else
	    direction = GV_BACKWARD;
	Vect_append_points(APoints, Points, direction);
	APoints->n_points--;	/* skip last point, avoids duplicates */
    }
    dig_line_box(APoints, &box);
    APoints->n_points++;	/* close polygon */

    dig_find_area_poly(APoints, &area_size);

    /* area_size = dig_find_poly_orientation(APoints); */
    /* area_size is not real area size, we are only interested in the sign */

    G_debug(3, "  area/isle size = %f", area_size);

    if (area_size > 0) {	/* CW: area */
	/* add area structure to plus */
	area = dig_add_area(plus, n_lines, lines, &box);
	if (area == -1) {	/* error */
	    Vect_close(Map);
	    G_fatal_error(_("Unable to add area (map closed, topo saved)"));
	}
	G_debug(3, "  -> area %d", area);
	return area;
    }
    else if (area_size < 0) {	/* CCW: island */
	isle = dig_add_isle(plus, n_lines, lines, &box);
	if (isle == -1) {	/* error */
	    Vect_close(Map);
	    G_fatal_error(_("Unable to add isle (map closed, topo saved)"));
	}
	G_debug(3, "  -> isle %d", isle);
	return -isle;
    }
    else {
	/* TODO: What to do with such areas? Should be areas/isles of size 0 stored,
	 *        so that may be found and cleaned by some utility
	 *  Note: it would be useful for vertical closed polygons, but such would be added twice
	 *        as area */
	G_warning(_("Area of size = 0.0 ignored"));
    }
    return 0;
}


/* for qsort */

typedef struct {
    int i;
    double size;
    struct bound_box box;
} BOX_SIZE;

static int sort_by_size(const void *a, const void *b)
{
    BOX_SIZE *as = (BOX_SIZE *)a;
    BOX_SIZE *bs = (BOX_SIZE *)b;
    
    if (as->size < bs->size)
	return -1;

    return (as->size > bs->size);
}


/*!
   \brief Find area outside island

   \param Map vector map
   \param isle isle id

   \return area id
   \return 0 if not found
 */
int Vect_isle_find_area(struct Map_info *Map, int isle)
{
    int i, line, sel_area, area, poly;
    const struct Plus_head *plus;
    struct P_line *Line;
    struct P_node *Node;
    struct P_isle *Isle;
    struct P_area *Area;
    struct P_topo_b *topo;
    double size, cur_size;
    struct bound_box box, *abox;
    static struct boxlist *List = NULL;
    static struct line_pnts *APoints;
    static BOX_SIZE *size_list;
    static int alloc_size_list = 0;
    static int debug_level = -1;

    if (debug_level == -1) {
	const char *dstr = G__getenv("DEBUG");

	if (dstr != NULL)
	    debug_level = atoi(dstr);
	else
	    debug_level = 0;
    }
    debug_level = 2;

    /* Note: We should check all isle points (at least) because if topology is not clean
     * and two areas overlap, isle which is not completely within area may be attached,
     * but it would take long time */

    G_debug(3, "Vect_isle_find_area () island = %d", isle);
    plus = &(Map->plus);

    if (plus->Isle[isle] == NULL) {
	G_warning(_("Request to find area outside nonexistent isle"));
	return 0;
    }

    if (!List) {
	List = Vect_new_boxlist(1);
	APoints = Vect_new_line_struct();
	alloc_size_list = 10;
	size_list = G_malloc(alloc_size_list * sizeof(BOX_SIZE));
    }

    Isle = plus->Isle[isle];
    line = abs(Isle->lines[0]);
    Line = plus->Line[line];
    topo = (struct P_topo_b *)Line->topo;
    Node = plus->Node[topo->N1];

    /* select areas by box */
    box.E = Node->x;
    box.W = Node->x;
    box.N = Node->y;
    box.S = Node->y;
    box.T = PORT_DOUBLE_MAX;
    box.B = -PORT_DOUBLE_MAX;
    Vect_select_areas_by_box(Map, &box, List);
    G_debug(3, "%d areas overlap island boundary point", List->n_values);

    Vect_get_isle_box(Map, isle, &box);

    /* sort areas by bbox size
     * get the smallest area that contains the isle
     * using the bbox size is working because if 2 areas both contain
     * the isle, one of these areas must be inside the other area
     * which means that the bbox of the outer area must be lager than
     * the bbox of the inner area, and equal bbox sizes are not possible */

    if (alloc_size_list < List->n_values) {
	alloc_size_list = List->n_values;
	size_list = G_realloc(size_list, alloc_size_list * sizeof(BOX_SIZE));
    }

    for (i = 0; i < List->n_values; i++) {
	size_list[i].i = List->id[i];
	abox = &List->box[i];
	size_list[i].box = List->box[i];
	size_list[i].size = (abox->N - abox->S) * (abox->E - abox->W);
    }

    if (List->n_values > 1) {
	if (List->n_values == 2) {
	    /* simple swap */
	    if (size_list[1].size < size_list[0].size) {
		size_list[0].i = List->id[1];
		size_list[1].i = List->id[0];
		size_list[0].box = List->box[1];
		size_list[1].box = List->box[0];
	    }
	}
	else
	    qsort(size_list, List->n_values, sizeof(BOX_SIZE), sort_by_size);
    }

    sel_area = 0;
    cur_size = -1;
    for (i = 0; i < List->n_values; i++) {
	area = size_list[i].i;
	G_debug(3, "area = %d", area);

	Area = plus->Area[area];

	/* Before other tests, simply exclude those areas inside isolated isles formed by one boundary */
	if (abs(Isle->lines[0]) == abs(Area->lines[0])) {
	    G_debug(3, "  area inside isolated isle");
	    continue;
	}

	/* Check box */
	/* Note: If build is run on large files of areas imported from nontopo format (shapefile)
	 * attaching of isles takes very long time because each area is also isle and select by
	 * box all overlapping areas selects all areas with box overlapping first node. 
	 * Then reading coordinates for all those areas would take a long time -> check first 
	 * if isle's box is completely within area box */

	abox = &size_list[i].box;

	if (box.E > abox->E || box.W < abox->W || box.N > abox->N ||
	    box.S < abox->S) {
	    G_debug(3, "  isle not completely inside area box");
	    continue;
	}

	poly = Vect_point_in_area_outer_ring(Node->x, Node->y, Map, area, abox);
	G_debug(3, "  poly = %d", poly);

	if (poly == 1) {	/* point in area, but node is not part of area inside isle (would be poly == 2) */

	    if (debug_level == 0) {
		G_debug(3, "Island %d in area %d", isle, sel_area);
		return area;
	    }
	    else {
		/* In rare case island is inside more areas in that case we have to calculate area
		 * of outer ring and take the smaller */
		if (sel_area == 0) {	/* first */
		    sel_area = area;
		}
		else {		/* is not first */
		    if (cur_size < 0) {	/* second area */
			/* This is slow, but should not be called often */
			Vect_get_area_points(Map, sel_area, APoints);
			/* G_begin_polygon_area_calculations();
			   cur_size =
			   G_area_of_polygon(APoints->x, APoints->y,
			   APoints->n_points); */
			/* this is faster, but there may be latlon problems: the poles */
			dig_find_area_poly(APoints, &cur_size);
			G_debug(3, "  first area size = %f (n points = %d)",
				cur_size, APoints->n_points);

		    }

		    Vect_get_area_points(Map, area, APoints);
		    /* size =
		       G_area_of_polygon(APoints->x, APoints->y,
		       APoints->n_points); */
		    /* this is faster, but there may be latlon problems: the poles */
		    dig_find_area_poly(APoints, &size);
		    G_debug(3, "  area size = %f (n points = %d)", size,
			    APoints->n_points);

		    if (size > 0 && size < cur_size) {
			sel_area = area;
			cur_size = size;
			/* this can not happen because the first area must be
			 * inside the second area because the node
			 * is inside both areas */
			G_warning(_("Larger bbox but smaller area!!!"));
		    }
		}
		G_debug(3, "sel_area = %d cur_size = %f", sel_area, cur_size);
	    }
	}
    }
    if (sel_area > 0) {
	G_debug(3, "Island %d in area %d", isle, sel_area);
    }
    else {
	G_debug(3, "Island %d is not in area", isle);
    }

    return sel_area;
}

/*!
   \brief (Re)Attach isle to area

   \param Map vector map
   \param isle isle id

   \return 0
 */
int Vect_attach_isle(struct Map_info *Map, int isle)
{
    int sel_area;
    struct P_isle *Isle;
    struct Plus_head *plus;

    /* Note!: If topology is not clean and areas overlap, one island
       may fall to more areas (partially or fully). Before isle is
       attached to area it must be check if it is not attached yet */
    G_debug(3, "Vect_attach_isle(): isle = %d", isle);

    plus = &(Map->plus);

    sel_area = Vect_isle_find_area(Map, isle);
    G_debug(3, "\tisle = %d -> area outside = %d", isle, sel_area);
    if (sel_area > 0) {
	Isle = plus->Isle[isle];
	if (Isle->area > 0) {
	    G_debug(3, "Attempt to attach isle %d to more areas "
		    "(=>topology is not clean)", isle);
	}
	else {
	    Isle->area = sel_area;
	    dig_area_add_isle(plus, sel_area, isle);
	}
    }
    return 0;
}

/*!
   \brief (Re)Attach isles to areas in given bounding box

   \param Map vector map
   \param box bounding box

   \return 0
 */
int Vect_attach_isles(struct Map_info *Map, const struct bound_box *box)
{
    int i, isle;
    static int first = TRUE;
    static struct boxlist *List;
    struct Plus_head *plus;

    G_debug(3, "Vect_attach_isles()");
      
    plus = &(Map->plus);

    if (first) {
	List = Vect_new_boxlist(FALSE);
	first = FALSE;
    }

    Vect_select_isles_by_box(Map, box, List);
    G_debug(3, "  number of isles to attach = %d", List->n_values);

    for (i = 0; i < List->n_values; i++) {
	isle = List->id[i];
	/* only attach isles that are not yet attached, see Vect_attach_isle() */
	if (plus->Isle[isle]->area == 0)
	    Vect_attach_isle(Map, isle);
    }
    return 0;
}

/*!
   \brief (Re)Attach centroids to areas in given bounding box

    Warning: If map is updated on level2, it may happen that
    previously correct island becomes incorrect. In that case,
    centroid of area forming the island is reattached to outer area,
    because island polygon is not excluded.
     
    <pre>
      +-----------+     +-----------+
      |   1       |     |   1       |
      | +---+---+ |     | +---+---+ |     
      | | 2 | 3 | |     | | 2 |     |   
      | | x |   | |  -> | | x |     |  
      | |   |   | |     | |   |     | 
      | +---+---+ |     | +---+---+ |
      |           |     |           |
      +-----------+     +-----------+
      centroid is       centroid is
      attached to 2     reattached to 1
    </pre>

    Because of this, when the centroid is reattached to another area,
    it is always necessary to check if original area exist, unregister
    centroid from previous area.  To simplify code, this is
    implemented so that centroid is always firs unregistered and if
    new area is found, it is registered again.
      
    This problem can be avoided altogether if properly attached
    centroids are skipped MM 2009

   \param Map vector map
   \param box bounding box

   \return 0
 */
int Vect_attach_centroids(struct Map_info *Map, const struct bound_box * box)
{
    int i, sel_area, centr;
    static int first = 1;
    static struct boxlist *List;
    static struct line_pnts *Points;
    struct P_area *Area;
    struct P_line *Line;
    struct P_topo_c *topo;
    struct Plus_head *plus;

    G_debug(3, "Vect_attach_centroids()");

    plus = &(Map->plus);

    if (first) {
	List = Vect_new_boxlist(0);
	Points = Vect_new_line_struct();
	first = 0;
    }

    Vect_select_lines_by_box(Map, box, GV_CENTROID, List);
    G_debug(3, "\tnumber of centroids to reattach = %d", List->n_values);
    for (i = 0; i < List->n_values; i++) {
	int orig_area;

	centr = List->id[i];
	Line = plus->Line[centr];
	topo = (struct P_topo_c *)Line->topo;

	/* only attach unregistered and duplicate centroids because 
	 * 1) all properly attached centroids are properly attached, really! Don't touch.
	 * 2) Vect_find_area() below does not always return the correct area
	 * 3) it's faster
	 */
	if (topo->area > 0)
	    continue;

	orig_area = topo->area;

	Vect_read_line(Map, Points, NULL, centr);
	if (Points->n_points < 1) {
	    /* try to get centroid from spatial index (OGR layers) */
	    int found;
	    struct boxlist list;
	    dig_init_boxlist(&list, TRUE);
	    Vect_select_lines_by_box(Map, box, GV_CENTROID, &list);

	    found = 0;
	    for (i = 0; i < list.n_values; i++) {
		if (list.id[i] == centr) {
		    found = i;
		    break;
		}
	    }
	    Vect_append_point(Points, list.box[found].E, list.box[found].N, 0.0);
	}
	sel_area = Vect_find_area(Map, Points->x[0], Points->y[0]);
	G_debug(3, "\tcentroid %d is in area %d", centr, sel_area);
	if (sel_area > 0) {
	    Area = plus->Area[sel_area];
	    if (Area->centroid == 0) {	/* first centroid */
		G_debug(3, "\tfirst centroid -> attach to area");
		Area->centroid = centr;
		topo->area = sel_area;

		if (sel_area != orig_area && plus->uplist.do_uplist)
		    dig_line_add_updated(plus, centr);
	    }
	    else if (Area->centroid != centr) {	/* duplicate centroid */
		/* Note: it cannot happen that Area->centroid == centr, because the centroid
		 * was not registered or a duplicate */
		G_debug(3, "\tduplicate centroid -> do not attach to area");
		topo->area = -sel_area;

		if (-sel_area != orig_area && plus->uplist.do_uplist)
		    dig_line_add_updated(plus, centr);
	    }
	}
    }

    return 0;
}

/*!
   \brief Build topology for vector map

   \param Map vector map

   \return 1 on success
   \return 0 on error
 */
int Vect_build(struct Map_info *Map)
{
    return Vect_build_partial(Map, GV_BUILD_ALL);
}

/*!
   \brief Extensive tests for correct topology

   - lines or boundaries of zero length
   - intersecting boundaries, ie. overlapping areas
   - areas without centroids that are not isles

   \param Map vector map
   \param[out] Err vector map where errors will be written or NULL

   \return 1 on success
   \return 0 on error
 */
int Vect_topo_check(struct Map_info *Map, struct Map_info *Err)
{
    int line, nlines;
    int nerrors, n_zero_lines, n_zero_boundaries;
    struct line_pnts *Points;
    struct line_cats *Cats;

    /* rebuild topology if needed */
    if (Vect_get_built(Map) != GV_BUILD_ALL) {
	Vect_build_partial(Map, GV_BUILD_NONE);
	Vect_build(Map);
    }

    G_message(_("Checking for topological errors..."));

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* lines or boundaries of zero length */
    n_zero_lines = n_zero_boundaries = 0;
    nlines = Vect_get_num_lines(Map);
    for (line = 1; line <= nlines; line++) {
	int type;

	if (!Vect_line_alive(Map, line))
	    continue;
	    
	type = Vect_get_line_type(Map, line);

	if (type & GV_LINES) {
	    double len;
	    
	    Vect_read_line(Map, Points, Cats, line);
	    len = Vect_line_length(Points);
	    
	    if (len == 0) {
		if (type & GV_LINE)
		    n_zero_lines++;
		else if (type & GV_BOUNDARY)
		    n_zero_boundaries++;
		    
		if (Err)
		    Vect_write_line(Err, type, Points, Cats);
	    }
	}
    }
    
    if (n_zero_lines)
	G_warning(_("Number of lines of length zero: %d"), n_zero_lines);
    if (n_zero_boundaries)
	G_warning(_("Number of boundaries of length zero: %d"), n_zero_boundaries);

    /* remaining checks are for areas only */
    if (Vect_get_num_primitives(Map, GV_BOUNDARY) == 0)
	return 1;

    /* intersecting boundaries -> overlapping areas */
    nerrors = Vect_check_line_breaks(Map, GV_BOUNDARY, Err);
    if (nerrors)
	G_warning(_("Number of boundary intersections: %d"), nerrors);

    /* areas without centroids that are not isles
     * only makes sense if all boundaries are correct */
    nerrors = 0;
    for (line = 1; line <= nlines; line++) {
	int type;
	
	if (!Vect_line_alive(Map, line))
	    continue;
	    
	type = Vect_get_line_type(Map, line);

	if (type == GV_BOUNDARY) {
	    struct P_topo_b *topo = (struct P_topo_b *)Map->plus.Line[line]->topo;

	    if (topo->left == 0 || topo->right == 0) {
		G_debug(3, "line = %d left = %d right = %d", line, 
			topo->left, topo->right);
		nerrors++;
	    }
	}
    }
    if (nerrors)
	G_warning(_("Skipping further checks because of incorrect boundaries"));
    else {
	int i, area, left, right, neighbour;
	int nareas = Vect_get_num_areas(Map);
	struct ilist *List = Vect_new_list();

	nerrors = 0;
	for (area = 1; area <= nareas; area++) {
	    if (!Vect_area_alive(Map, area))
		continue;
	    line = Vect_get_area_centroid(Map, area);
	    if (line != 0)
		continue;   /* has centroid */

	    Vect_get_area_boundaries(Map, area, List);
	    for (i = 0; i < List->n_values; i++) {
		line = List->value[i];
		Vect_get_line_areas(Map, abs(line), &left, &right);
		if (line > 0)
		    neighbour = left;
		else
		    neighbour = right;
		    
		if (neighbour < 0) {
		    neighbour = Vect_get_isle_area(Map, abs(neighbour));
		    if (!neighbour) {
			/* borders outer void */
			nerrors++;
			if (Err) {
			    Vect_read_line(Map, Points, Cats, abs(line));
			    Vect_write_line(Err, GV_BOUNDARY, Points, Cats);
			}
		    }
		    /* else neighbour is > 0, check below */
		}
		if (neighbour > 0) {
		    if (Vect_get_area_centroid(Map, neighbour) == 0) {
			/* neighbouring area does not have a centroid either */
			nerrors++;
			if (Err) {
			    Vect_read_line(Map, Points, Cats, abs(line));
			    Vect_write_line(Err, GV_BOUNDARY, Points, Cats);
			}
		    }
		}
	    }
	}
	Vect_destroy_list(List);

	if (nerrors)
	    G_warning(_("Number of redundant holes: %d"), 
	              nerrors);
    }

    /* what else ? */

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return 1;
}

/*!
   \brief Return current highest built level (part)

   \param Map vector map

   \return current highest built level
 */
int Vect_get_built(const struct Map_info *Map)
{
    return Map->plus.built;
}

/*!
  \brief Downgrade build level (for internal use only)

  See Vect_build_nat(), Vect__build_sfa(), and Vect_build_pg() for
  implementation issues.

  \param Map pointer to Map_info
  \param build
*/
void Vect__build_downgrade(struct Map_info *Map, int build)
{
    int line;
    struct Plus_head *plus;
    struct P_line *Line;
    
    plus = &(Map->plus);
    
    /* lower level request - release old sources (this also
       initializes structures and numbers of elements) */
    if (plus->built >= GV_BUILD_CENTROIDS && build < GV_BUILD_CENTROIDS) {
        /* reset info about areas stored for centroids */
        for (line = 1; line <= plus->n_lines; line++) {
            Line = plus->Line[line];
            if (Line && Line->type == GV_CENTROID) {
                struct P_topo_c *topo = (struct P_topo_c *)Line->topo;
                topo->area = 0;
            }
        }
        dig_free_plus_areas(plus);
        dig_spidx_free_areas(plus);
        dig_free_plus_isles(plus);
        dig_spidx_free_isles(plus);
    }
    
    if (plus->built >= GV_BUILD_AREAS && build < GV_BUILD_AREAS) {
        /* reset info about areas stored for lines */
        for (line = 1; line <= plus->n_lines; line++) {
            Line = plus->Line[line];
            if (Line && Line->type == GV_BOUNDARY) {
                struct P_topo_b *topo = (struct P_topo_b *)Line->topo;
                topo->left = 0;
                topo->right = 0;
            }
        }
        dig_free_plus_areas(plus);
        dig_spidx_free_areas(plus);
        dig_free_plus_isles(plus);
        dig_spidx_free_isles(plus);
    }
    
    if (plus->built >= GV_BUILD_BASE && build < GV_BUILD_BASE) {
        dig_free_plus_nodes(plus);
        dig_spidx_free_nodes(plus);
        dig_free_plus_lines(plus);
        dig_spidx_free_lines(plus);
    }

    plus->built = build;
}

/*!
   \brief Build partial topology for vector map.

   Should only be used in special cases of vector processing.

   This functions optionally builds only some parts of
   topology. Highest level is specified by build parameter which may
   be:
    - GV_BUILD_NONE - nothing is build
    - GV_BUILD_BASE - basic topology, nodes, lines, spatial index;
    - GV_BUILD_AREAS - build areas and islands, but islands are not attached to areas;
    - GV_BUILD_ATTACH_ISLES - attach islands to areas;
    - GV_BUILD_CENTROIDS - assign centroids to areas;
    - GV_BUILD_ALL - top level, the same as GV_BUILD_CENTROIDS.
    
   If functions is called with build lower than current value of the
   Map, the level is downgraded to requested value.

   All calls to Vect_write_line(), Vect_rewrite_line(),
   Vect_delete_line() respect the last value of build used in this
   function.

   Note that the functions has effect only if requested level is
   higher than current level, to rebuild part of topology, call first
   downgrade and then upgrade, for example:

   - Vect_build()
   - Vect_build_partial(,GV_BUILD_BASE,)
   - Vect_build_partial(,GV_BUILD_AREAS,) 

   \param Map vector map
   \param build highest level of build

   \return 1 on success
   \return 0 on error
 */
int Vect_build_partial(struct Map_info *Map, int build)
{
    struct Plus_head *plus;
    int ret;

    G_debug(3, "Vect_build(): build = %d", build);

    /* If topology is already build (map on > level 2), set level to 1
     * so that lines will be read by V1_read_ (all lines) */
    Map->level = 1; /* may be not needed, because V1_read is used
		       directly by Vect_build_ */
    if (Map->format != GV_FORMAT_OGR_DIRECT)
	Map->support_updated = TRUE;

    if (!Map->plus.Spidx_built)
	Vect_open_sidx(Map, 2);

    plus = &(Map->plus);
    if (build > GV_BUILD_NONE) {
	G_message(_("Building topology for vector map <%s>..."),
		  Vect_get_full_name(Map));
    }
    plus->with_z = Map->head.with_z;
    plus->spidx_with_z = Map->head.with_z;

    if (build == GV_BUILD_ALL) {
	dig_cidx_free(plus);	/* free old (if any) category index */
	dig_cidx_init(plus);
    }

    ret = ((*Build_array[Map->format]) (Map, build));

    if (ret == 0) {
	return 0;
    }

    if (build > GV_BUILD_NONE) {
	G_verbose_message(_("Topology was built"));
    }

    Map->level = LEVEL_2;
    plus->mode = GV_MODE_WRITE;

    if (build == GV_BUILD_ALL) {
	plus->cidx_up_to_date = TRUE;	/* category index was build */
	dig_cidx_sort(plus);
    }

    if (build > GV_BUILD_NONE) {
	G_message(_("Number of nodes: %d"), plus->n_nodes);
	G_message(_("Number of primitives: %d"), plus->n_lines);
	G_message(_("Number of points: %d"), plus->n_plines);
	G_message(_("Number of lines: %d"), plus->n_llines);
	G_message(_("Number of boundaries: %d"), plus->n_blines);
	G_message(_("Number of centroids: %d"), plus->n_clines);

	if (plus->n_flines > 0)
	    G_message(_("Number of faces: %d"), plus->n_flines);

	if (plus->n_klines > 0)
	    G_message(_("Number of kernels: %d"), plus->n_klines);
    }

    if (plus->built >= GV_BUILD_AREAS) {
	int line, nlines, area, nareas, err_boundaries, err_centr_out,
	    err_centr_dupl, err_nocentr;
	struct P_line *Line;
	struct Plus_head *Plus;

	/* Count errors (it does not take much time comparing to build process) */
	Plus = &(Map->plus);
	nlines = Vect_get_num_lines(Map);
	err_boundaries = err_centr_out = err_centr_dupl = 0;
	for (line = 1; line <= nlines; line++) {
	    Line = Plus->Line[line];
	    if (!Line)
		continue;
	    if (Line->type == GV_BOUNDARY) {
		struct P_topo_b *topo = (struct P_topo_b *)Line->topo;

		if (topo->left == 0 || topo->right == 0) {
		    G_debug(3, "line = %d left = %d right = %d", line, 
			    topo->left, topo->right);
		    err_boundaries++;
		}
	    }
	    if (Line->type == GV_CENTROID) {
		struct P_topo_c *topo = (struct P_topo_c *)Line->topo;

		if (topo->area == 0)
		    err_centr_out++;
		else if (topo->area < 0)
		    err_centr_dupl++;
	    }
	}

	err_nocentr = 0;
	nareas = Vect_get_num_areas(Map);
	for (area = 1; area <= nareas; area++) {
	    if (!Vect_area_alive(Map, area))
		continue;
	    line = Vect_get_area_centroid(Map, area);
	    if (line == 0)
		err_nocentr++;
	}

	G_message(_("Number of areas: %d"), plus->n_areas);
	G_message(_("Number of isles: %d"), plus->n_isles);

#if 0
	/* not an error, message disabled to avoid confusion */
	if (err_nocentr)
	    G_message(_("Number of areas without centroid: %d"),
		      err_nocentr);
#endif

	if (plus->n_clines > plus->n_areas)
	    G_warning(_("Number of centroids exceeds number of areas: %d > %d"),
		      plus->n_clines, plus->n_areas);

	if (err_boundaries)
	    G_warning(_("Number of incorrect boundaries: %d"),
		      err_boundaries);

	if (err_centr_out)
	    G_warning(_("Number of centroids outside area: %d"),
		      err_centr_out);

	if (err_centr_dupl)
	    G_warning(_("Number of duplicate centroids: %d"), err_centr_dupl);

    }
    else if (build > GV_BUILD_NONE) {
	G_message(_("Number of areas: -"));
	G_message(_("Number of isles: -"));
    }
    return 1;
}

/*!
   \brief Save topology file for vector map

   \param Map pointer to Map_info structure

   \return 1 on success
   \return 0 on error
 */
int Vect_save_topo(struct Map_info *Map)
{
    struct Plus_head *plus;
    char buf[GPATH_MAX];
    struct gvfile fp;

    G_debug(1, "Vect_save_topo()");

    plus = &(Map->plus);

    /*  write out all the accumulated info to the plus file  */
    sprintf(buf, "%s/%s", GV_DIRECTORY, Map->name);
    dig_file_init(&fp);
    fp.file = G_fopen_new(buf, GV_TOPO_ELEMENT);
    if (fp.file == NULL) {
	G_warning(_("Unable to create topo file for vector map <%s>"), Map->name);
	return 0;
    }

    /* set portable info */
    dig_init_portable(&(plus->port), dig__byte_order_out());

    if (0 > dig_write_plus_file(&fp, plus)) {
	G_warning(_("Error writing out topo file"));
	return 0;
    }

    fclose(fp.file);

    return 1;
}

/*!
   \brief Dump topology to file

   \param Map vector map
   \param out file for output (stdout/stderr for example)

   \return 1 on success
   \return 0 on error
 */
int Vect_topo_dump(const struct Map_info *Map, FILE *out)
{
    int i, j, line, isle;
    float angle_deg;
    struct P_node *Node;
    struct P_line *Line;
    struct P_area *Area;
    struct P_isle *Isle;
    struct bound_box box;
    const struct Plus_head *plus;

    plus = &(Map->plus);
    
    fprintf(out, "---------- TOPOLOGY DUMP ----------\n");
    if (Map->format == GV_FORMAT_NATIVE)
        fprintf(out, "-------------- NATIVE -------------\n");
    else if (Map->format == GV_FORMAT_POSTGIS &&
             Map->fInfo.pg.toposchema_name)
        fprintf(out, "------------- POSTGIS -------------\n");
    else
        fprintf(out, "-------------- PSEUDO -------------\n");
    
    /* box */
    Vect_box_copy(&box, &(plus->box));
    fprintf(out, "N,S,E,W,T,B: %f, %f, %f, %f, %f, %f\n", box.N, box.S,
	    box.E, box.W, box.T, box.B);

    fprintf(out, SEP);
    
    /* nodes */
    fprintf(out, "Nodes (%d nodes, alive + dead):\n", plus->n_nodes);
    for (i = 1; i <= plus->n_nodes; i++) {
	if (plus->Node[i] == NULL) {
	    continue;
	}
	Node = plus->Node[i];
	fprintf(out, "node = %d, n_lines = %d, xyz = %f, %f, %f\n", i,
		Node->n_lines, Node->x, Node->y, Node->z);
	for (j = 0; j < Node->n_lines; j++) {
	    line = Node->lines[j];
	    Line = plus->Line[abs(line)];
            angle_deg = (Node->angles[j] * 180) / M_PI;
            if (angle_deg < 0)
                angle_deg += 360;
	    fprintf(out, "  line = %3d, type = %d, angle = %f (%.4f)\n", line,
		    Line->type, Node->angles[j], angle_deg);
	}
    }

    fprintf(out, SEP);
    
    /* lines */
    fprintf(out, "Lines (%d lines, alive + dead):\n", plus->n_lines);
    for (i = 1; i <= plus->n_lines; i++) {
	if (plus->Line[i] == NULL) {
	    continue;
	}
	Line = plus->Line[i];
	if (Line->type == GV_POINT) {
	    fprintf(out, "line = %d, type = %d, offset = %lu\n",
		    i, Line->type, (unsigned long)Line->offset);
	}
	else if (Line->type == GV_CENTROID) {
	    struct P_topo_c *topo = (struct P_topo_c *)Line->topo;

	    fprintf(out, "line = %d, type = %d, offset = %lu, area = %d\n",
		    i, Line->type, (unsigned long)Line->offset, topo->area);
	}
	else if (Line->type == GV_LINE) {
	    struct P_topo_l *topo = (struct P_topo_l *)Line->topo;

	    fprintf(out, "line = %d, type = %d, offset = %lu, n1 = %d, n2 = %d\n",
		    i, Line->type, (unsigned long)Line->offset, topo->N1,
		    topo->N2);
	}
	else if (Line->type == GV_BOUNDARY) {
	    struct P_topo_b *topo = (struct P_topo_b *)Line->topo;

	    fprintf(out, "line = %d, type = %d, offset = %lu, n1 = %d, n2 = %d, "
		    "left = %d, right = %d\n",
		    i, Line->type, (unsigned long)Line->offset, topo->N1,
		    topo->N2, topo->left, topo->right);
	}
	else if (Line->type == GV_FACE) {
	    struct P_topo_f *topo = (struct P_topo_f *)Line->topo;

	    fprintf(out, "line = %d, type = %d, offset = %lu, e1 = %d, e2 = %d, "
		    "e3 = %d, left = %d, right = %d\n",
		    i, Line->type, (unsigned long)Line->offset, topo->E[0],
		    topo->E[1], topo->E[2], topo->left, topo->right);
	}
	else if (Line->type == GV_KERNEL) {
	    struct P_topo_k *topo = (struct P_topo_k *)Line->topo;

	    fprintf(out, "line = %d, type = %d, offset = %lu, volume = %d",
		    i, Line->type, (unsigned long)Line->offset, topo->volume);
	}
    }

    fprintf(out, SEP);
    
    /* areas */
    fprintf(out, "Areas (%d areas, alive + dead):\n", plus->n_areas);
    for (i = 1; i <= plus->n_areas; i++) {
	if (plus->Area[i] == NULL) {
	    continue;
	}
	Area = plus->Area[i];

	fprintf(out, "area = %d, n_lines = %d, n_isles = %d centroid = %d\n",
		i, Area->n_lines, Area->n_isles, Area->centroid);

	for (j = 0; j < Area->n_lines; j++) {
	    line = Area->lines[j];
	    Line = plus->Line[abs(line)];
	    fprintf(out, "  line = %3d\n", line);
	}
	for (j = 0; j < Area->n_isles; j++) {
	    isle = Area->isles[j];
	    fprintf(out, "  isle = %3d\n", isle);
	}
    }

    fprintf(out, SEP);
    
    /* isles */
    fprintf(out, "Islands (%d islands, alive + dead):\n", plus->n_isles);
    for (i = 1; i <= plus->n_isles; i++) {
	if (plus->Isle[i] == NULL) {
	    continue;
	}
	Isle = plus->Isle[i];

	fprintf(out, "isle = %d, n_lines = %d area = %d\n", i, Isle->n_lines,
		Isle->area);

	for (j = 0; j < Isle->n_lines; j++) {
	    line = Isle->lines[j];
	    Line = plus->Line[abs(line)];
	    fprintf(out, "  line = %3d\n", line);
	}
    }

    return 1;
}

/*!
   \brief Create spatial index if necessary.

   To be used in modules.
   Map must be opened on level 2.

   \param[in,out] Map pointer to vector map

   \return 0 OK
   \return 1 error
 */
int Vect_build_sidx(struct Map_info *Map)
{
    if (Map->level < 2) {
	G_fatal_error(_("Unable to build spatial index from topology, "
			"vector map is not opened at topology level 2"));
    }
    if (!Map->plus.Spidx_built) {
	return Vect_build_sidx_from_topo(Map);
    }
    return 0;
}

/*!
   \brief Create spatial index from topology if necessary (not longer
   supported)

   \param Map pointer to vector map

   \return 1
 */
int Vect_build_sidx_from_topo(const struct Map_info *Map)
{

    G_debug(3, "Vect_build_sidx_from_topo(): name=%s",
	    Vect_get_full_name(Map));

    G_warning(_("%s is no longer supported"), "Vect_build_sidx_from_topo()");

    return 1;
}

/*!
   \brief Save spatial index file for vector map

   \param Map vector map

   \return 1 on success
   \return 0 on error
 */
int Vect_save_sidx(struct Map_info *Map)
{
    struct Plus_head *plus;
    char fname[GPATH_MAX], buf[GPATH_MAX];

    G_debug(1, "Vect_save_spatial_index()");

    plus = &(Map->plus);

    if (!plus->Spidx_built) {
	G_warning(_("Spatial index not available, can not be saved"));
	return 0;
    }

    /* new or update mode ? */
    if (plus->Spidx_new == TRUE) {
	/*  write out rtrees to sidx file  */
	sprintf(buf, "%s/%s", GV_DIRECTORY, Map->name);
	G_file_name(fname, buf, GV_SIDX_ELEMENT, Map->mapset);
	G_debug(1, "Open sidx: %s", fname);
	dig_file_init(&(plus->spidx_fp));
	plus->spidx_fp.file = fopen(fname, "w+");
	if (plus->spidx_fp.file == NULL) {
	    G_warning(_("Unable open spatial index file for write <%s>"),
		      fname);
	    return 0;
	}

	/* set portable info */
	dig_init_portable(&(plus->spidx_port), dig__byte_order_out());

	if (0 > dig_Wr_spidx(&(plus->spidx_fp), plus)) {
	    G_warning(_("Error writing out spatial index file"));
	    return 0;
	}
	Map->plus.Spidx_new = FALSE;
    }

    fclose(Map->plus.spidx_fp.file);

    Map->plus.Spidx_built = FALSE;

    return 1;
}

/*!
   \brief Dump spatial index to file

   \param Map vector map
   \param out file for output (stdout/stderr for example)

   \return 1 on success
   \return 0 on error
 */
int Vect_sidx_dump(const struct Map_info *Map, FILE * out)
{
    if (!(Map->plus.Spidx_built)) {
	Vect_build_sidx_from_topo(Map);
    }

    fprintf(out, "---------- SPATIAL INDEX DUMP ----------\n");

    dig_dump_spidx(out, &(Map->plus));

    return 1;
}
