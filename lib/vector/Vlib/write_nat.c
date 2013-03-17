/*!
   \file lib/vector/Vlib/write_nat.c

   \brief Vector library - write/modify vector feature (native format)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
   \author V*_restore_line() by Martin Landa <landa.martin gmail.com> (2008)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

static off_t V1__rewrite_line_nat(struct Map_info *, off_t, int,
				  const struct line_pnts *, const struct line_cats *);

/*! 
  \brief Deletes area (i.e. centroid) categories from category
  index (internal use only)

  Call G_fatal_error() when area do not exits.
  
  \param Map pointer to Map_info structure
  \param area area id
*/
static void V2__delete_area_cats_from_cidx_nat(struct Map_info *Map, int area)
{
    int i;
    struct P_area *Area;
    static struct line_cats *Cats = NULL;

    G_debug(3, "V2__delete_area_cats_from_cidx_nat(), area = %d", area);

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

  Call G_fatal_error() when area do not exits.
  
  \param Map pointer to Map_info structure
  \param area area id
*/
static void V2__add_area_cats_to_cidx_nat(struct Map_info *Map, int area)
{
    int i;
    struct P_area *Area;
    static struct line_cats *Cats = NULL;

    G_debug(3, "V2__add_area_cats_to_cidx_nat(), area = %d", area);

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
  \brief Add feature (line) to topo file (internal use only)

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
 
   - re-attach all centroids/isles inside new area(s)
   - attach new isle to area outside

  2) line is closed ring (node at the end is new, so it is not case above)
    - build new area/isle
    - check if it is island or contains island(s)
    - re-attach all centroids/isles inside new area(s)
    - attach new isle to area outside
    
    Note that 1) and 2) is done by the same code.

    \param Map pointer to Map_info structure
    \param line feature id
    \param points pointer to line_pnts structure (feature's geometry)
    \param cats pointer to line_cats structure (feature's categories)
*/
void V2__add_line_to_topo_nat(struct Map_info *Map, int line,
                              const struct line_pnts *points, const struct line_cats *cats,
                              int (*external_routine) (const struct Map_info *, int))
{
    int first, s, n, i;
    int type, node, next_line, area, side, sel_area, new_area[2];

    struct Plus_head *plus;
    struct P_line *Line, *NLine;
    struct P_node *Node;
    struct P_area *Area;
    
    struct bound_box box, abox;
    
    G_debug(3, "V2__add_line_to_topo_nat(), line = %d", line);

    plus = &(Map->plus);
    Line = plus->Line[line];
    type = Line->type;

    if (plus->built >= GV_BUILD_AREAS &&
	type == GV_BOUNDARY) {
	struct P_topo_b *topo = (struct P_topo_b *)Line->topo;
	
	/* Delete neighbour areas/isles */
	first = TRUE;
	for (s = 0; s < 2; s++) {	/* for each node */
	    node = (s == 0 ? topo->N1 : topo->N2);
	    G_debug(3,
		    "  delete neighbour areas/isles: %s node = %d",
		    (s == 0 ? "first" : "second"), node);
	    Node = plus->Node[node];
	    n = 0;
	    for (i = 0; i < Node->n_lines; i++) {
		NLine = plus->Line[abs(Node->lines[i])];
		if (NLine->type == GV_BOUNDARY)
		    n++;
	    }
	    
	    G_debug(3, "  number of boundaries at node = %d", n);
	    if (n > 2) {
		/* more than 2 boundaries at node ( >= 2 old + 1 new ) */
		/* Line above (to the right), it is enough to check to
		   the right, because if area/isle exists it is the
		   same to the left */
		if (!s)
		    next_line =
			dig_angle_next_line(plus, line, GV_RIGHT,
					    GV_BOUNDARY, NULL);
		else
		    next_line =
			dig_angle_next_line(plus, -line, GV_RIGHT,
					    GV_BOUNDARY, NULL);
		
		if (next_line != 0) {	/* there is a boundary to the right */
		    NLine = plus->Line[abs(next_line)];
		    topo = (struct P_topo_b *)NLine->topo;
		    if (next_line > 0)	/* the boundary is connected by 1. node */
			/* we are interested just in this side (close to our line) */
			area = topo->right;	
		    else if (next_line < 0)	/* the boundary is connected by 2. node */
			area = topo->left;
		    
		    G_debug(3, "  next_line = %d area = %d", next_line,
			    area);
		    if (area > 0) {	/* is area */
			Vect_get_area_box(Map, area, &box);
			if (first) {
			    Vect_box_copy(&abox, &box);
			    first = FALSE;
			}
			else
			    Vect_box_extend(&abox, &box);
			
			if (plus->update_cidx) {
			    V2__delete_area_cats_from_cidx_nat(Map, area);
			}
			dig_del_area(plus, area);
                        if (external_routine) /* call external subroutine if defined */
                            external_routine(Map, area);
		    }
		    else if (area < 0) {	/* is isle */
			dig_del_isle(plus, -area);
                        if (external_routine)  /* call external subroutine if defined */
                            external_routine(Map, area);
		    }
		}
	    }
	}
	/* Build new areas/isles.
	 * It's true that we deleted also adjacent areas/isles, but
	 * if they form new one our boundary must participate, so
	 * we need to build areas/isles just for our boundary */
	for (s = 0; s < 2; s++) {
	    side = (s == 0 ? GV_LEFT : GV_RIGHT);
	    area = Vect_build_line_area(Map, line, side);
	    
	    if (area > 0) {	/* area */
		Vect_get_area_box(Map, area, &box);
		if (first) {
		    Vect_box_copy(&abox, &box);
		    first = FALSE;
		}
		else
		    Vect_box_extend(&abox, &box);
	    }
	    else if (area < 0) {
		/* isle -> must be attached -> add to abox */
		Vect_get_isle_box(Map, -area, &box);
		if (first) {
		    Vect_box_copy(&abox, &box);
		    first = FALSE;
		}
		else
		    Vect_box_extend(&abox, &box);
	    }
	    new_area[s] = area;
	}
	/* Reattach all centroids/isles in deleted areas + new area.
	 * Because isles are selected by box it covers also possible
	 * new isle created above */
	if (!first) { /* i.e. old area/isle was deleted or new one created */
	    /* Reattach isles */
	    if (plus->built >= GV_BUILD_ATTACH_ISLES)
		Vect_attach_isles(Map, &abox);
	    
	    /* Reattach centroids */
	    if (plus->built >= GV_BUILD_CENTROIDS)
		Vect_attach_centroids(Map, &abox);
	}
	/* Add to category index */
	if (plus->update_cidx) {
	    for (s = 0; s < 2; s++) {
		if (new_area[s] > 0) {
		    V2__add_area_cats_to_cidx_nat(Map, new_area[s]);
		}
	    }
	}
    }
    
    /* Attach centroid */
    if (plus->built >= GV_BUILD_CENTROIDS) {
	struct P_topo_c *topo;

	if (type == GV_CENTROID) {
	    sel_area = Vect_find_area(Map, points->x[0], points->y[0]);
	    G_debug(3, "  new centroid %d is in area %d", line, sel_area);
	    if (sel_area > 0) {
		Area = plus->Area[sel_area];
		Line = plus->Line[line];
		topo = (struct P_topo_c *)Line->topo;
		if (Area->centroid == 0) {	/* first centroid */
		    G_debug(3, "  first centroid -> attach to area");
		    Area->centroid = line;
		    topo->area = sel_area;
		    if (plus->update_cidx) {
			V2__add_area_cats_to_cidx_nat(Map, sel_area);
		    }
		}
		else {		/* duplicate centroid */
		    G_debug(3,
			    "  duplicate centroid -> do not attach to area");
		    topo->area = -sel_area;
		}
	    }
	}
    }

    /* Add category index */
    if (cats) {
        for (i = 0; i < cats->n_cats; i++) {
            dig_cidx_add_cat_sorted(plus, cats->field[i], cats->cat[i], line,
                                    type);
        }
    }
    
    return;
}

/*!
  \brief Writes feature to 'coor' file (level 1)
  
  \param Map pointer to Map_info structure
  \param type feature type (GV_POINT, GV_LINE, ...)
  \param points feature geometry
  \param cats feature categories
  
  \return feature offset into file
  \return -1 on error
*/
off_t V1_write_line_nat(struct Map_info *Map,
		       int type, const struct line_pnts *points, const struct line_cats *cats)
{
    off_t offset;

    if (dig_fseek(&(Map->dig_fp), 0L, SEEK_END) == -1)	/* set to end of file */
	return -1;

    offset = dig_ftell(&(Map->dig_fp));
    if (offset == -1)
	return -1;

    return V1__rewrite_line_nat(Map, offset, type, points, cats);
}

/*!
  \brief Writes feature to 'coor' file at topological level (internal use only)
  
  \param Map pointer to Map_info structure
  \param type feature type (GV_POINT, GV_LINE, ...)
  \param points feature geometry
  \param cats feature categories
  
  \return new feature id
  \return -1 on error
*/
off_t V2_write_line_nat(struct Map_info *Map, int type,
			const struct line_pnts *points, const struct line_cats *cats)
{
    int line;
    off_t offset;
    struct Plus_head *plus;
    struct bound_box box;

    line = 0;
    
    G_debug(3, "V2_write_line_nat()");
    offset = V1_write_line_nat(Map, type, points, cats);
    if (offset < 0)
	return -1;

    /* Update topology */
    plus = &(Map->plus);
    /* Add line */
    if (plus->built >= GV_BUILD_BASE) {
	dig_line_box(points, &box);
	line = dig_add_line(plus, type, points, &box, offset);
	G_debug(3, "  line added to topo with id = %d", line);
	if (line == 1)
	    Vect_box_copy(&(plus->box), &box);
	else
	    Vect_box_extend(&(plus->box), &box);
    }

    V2__add_line_to_topo_nat(Map, line, points, cats, NULL);

    G_debug(3, "updated lines : %d , updated nodes : %d", plus->uplist.n_uplines,
	    plus->uplist.n_upnodes);

    /* returns int line, but is defined as off_t for compatibility with
     * Vect_write_line_array in write.c */
    
    return line;
}

/*!
  \brief Rewrites feature at the given offset (level 1)
  
  If the number of points or cats differs from the original one or
  the type is changed: GV_POINTS -> GV_LINES or GV_LINES ->
  GV_POINTS, the old one is deleted and the new is appended to the
  end of the file.
  
  Old feature is deleted (marked as dead), new feature written.
  
  \param Map pointer to Map_info structure
  \param offset feature offset
  \param type feature type (GV_POINT, GV_LINE, ...)
  \param points feature geometry
  \param cats feature categories
  
  \return feature offset (rewriten feature)
  \return -1 on error
*/
off_t V1_rewrite_line_nat(struct Map_info *Map, int line, int type, off_t offset,
			  const struct line_pnts *points, const struct line_cats *cats)
{
    int old_type;
    struct line_pnts *old_points;
    struct line_cats *old_cats;
    off_t new_offset;
    
    G_debug(3, "V1_rewrite_line_nat(): line = %d offset = %lu",
	    line, (unsigned long) offset);

    /* TODO: enable points and cats == NULL  */

    /* First compare numbers of points and cats with tha old one */
    old_points = Vect_new_line_struct();
    old_cats = Vect_new_cats_struct();

    old_type = V1_read_line_nat(Map, old_points, old_cats, offset);
    if (old_type == -1)
	return (-1);		/* error */

    if (old_type != -2		/* EOF -> write new line */
	&& points->n_points == old_points->n_points
	&& cats->n_cats == old_cats->n_cats
	&& (((type & GV_POINTS) && (old_type & GV_POINTS))
	    || ((type & GV_LINES) && (old_type & GV_LINES)))) {

	/* equal -> overwrite the old */
	return V1__rewrite_line_nat(Map, offset, type, points, cats);
    }
    else {
	/* differ -> delete the old and append new */
	/* delete old */
	V1_delete_line_nat(Map, offset);

	/* write new */
	if (dig_fseek(&(Map->dig_fp), 0L, SEEK_END) == -1)	/*  end of file */
	    return -1;

	new_offset = dig_ftell(&(Map->dig_fp));
	if (new_offset == -1)
	    return -1;

	return V1__rewrite_line_nat(Map, new_offset, type, points, cats);
    }
}

/*!
  \brief Rewrites feature to 'coor' file at topological level (internal use only)
  
  \param Map pointer to Map_info structure
  \param type feature type  (GV_POINT, GV_LINE, ...)
  \param line feature id
  \param points feature geometry
  \param cats feature categories
  
  \return offset where line was rewritten
  \return -1 on error
*/
off_t V2_rewrite_line_nat(struct Map_info *Map, int line, int type, off_t old_offset,
			  const struct line_pnts *points, const struct line_cats *cats)
{
    /* TODO: this is just quick shortcut because we have already V2_delete_nat()
     *        and V2_write_nat() this function first deletes old line
     *        and then writes new one. It is not very effective if number of points
     *        and cats was not changed or topology is not changed (nodes not moved,
     *        angles not changed etc.) */

    off_t offset;
    struct Plus_head *plus;
    struct bound_box box;

    V2_delete_line_nat(Map, line);
    
    G_debug(3, "V2_write_line_nat(), line = %d", line);
    offset = V1_rewrite_line_nat(Map, line, type, old_offset, points, cats);
    if (offset < 0)
	return -1;

    /* Update topology */
    plus = &(Map->plus);
    /* Add line */
    if (plus->built >= GV_BUILD_BASE) {
	dig_line_box(points, &box);
	line = dig_add_line(plus, type, points, &box, offset);
	G_debug(3, "  line added to topo with id = %d", line);
	if (line == 1)
	    Vect_box_copy(&(plus->box), &box);
	else
	    Vect_box_extend(&(plus->box), &box);
    }

    V2__add_line_to_topo_nat(Map, line, points, cats, NULL);

    G_debug(3, "updated lines : %d , updated nodes : %d", plus->uplist.n_uplines,
	    plus->uplist.n_upnodes);

    /* returns int line, but is defined as off_t for compatibility with
     * Vect_rewrite_line_array in write.c */
    
    return line;
}

/*!
  \brief Rewrites feature at the given offset.
  
  \param Map pointer to Map_info structure
  \param offset feature offset
  \param type feature type  (GV_POINT, GV_LINE, ...)
  \param points feature geometry
  \param cats feature categories
  
  \return feature offset
  \return -1 on error
*/
off_t V1__rewrite_line_nat(struct Map_info *Map,
			   off_t offset, int type,
			   const struct line_pnts *points, const struct line_cats *cats)
{
    int i, n_points;
    char rhead, nc;
    short field;
    struct gvfile *dig_fp;

    dig_set_cur_port(&(Map->head.port));
    dig_fp = &(Map->dig_fp);

    if (dig_fseek(dig_fp, offset, 0) == -1)
	return -1;

    /* first byte:   0 bit: 1 - alive, 0 - dead
     *                1 bit: 1 - categories, 0 - no category
     *              2-3 bit: store type
     *              4-5 bit: reserved for store type expansion
     *              6-7 bit: not used  
     */

    rhead = (char)dig_type_to_store(type);
    rhead <<= 2;
    if (cats->n_cats > 0) {
	rhead |= 0x02;
    }
    rhead |= 0x01;		/* written/rewritten is always alive */

    if (0 >= dig__fwrite_port_C(&rhead, 1, dig_fp)) {
	return -1;
    }

    if (cats->n_cats > 0) {
	if (Map->head.Version_Minor == 1) {	/* coor format 5.1 */
	    if (0 >= dig__fwrite_port_I(&(cats->n_cats), 1, dig_fp))
		return -1;
	}
	else {			/* coor format 5.0 */
	    nc = (char)cats->n_cats;
	    if (0 >= dig__fwrite_port_C(&nc, 1, dig_fp))
		return -1;
	}

	if (cats->n_cats > 0) {
	    if (Map->head.Version_Minor == 1) {	/* coor format 5.1 */
		if (0 >=
		    dig__fwrite_port_I(cats->field, cats->n_cats, dig_fp))
		    return -1;
	    }
	    else {		/* coor format 5.0 */
		for (i = 0; i < cats->n_cats; i++) {
		    field = (short)cats->field[i];
		    if (0 >= dig__fwrite_port_S(&field, 1, dig_fp))
			return -1;
		}
	    }
	    if (0 >= dig__fwrite_port_I(cats->cat, cats->n_cats, dig_fp))
		return -1;
	}
    }

    if (type & GV_POINTS) {
	n_points = 1;
    }
    else {
	n_points = points->n_points;
	if (0 >= dig__fwrite_port_I(&n_points, 1, dig_fp))
	    return -1;
    }

    if (0 >= dig__fwrite_port_D(points->x, n_points, dig_fp))
	return -1;
    if (0 >= dig__fwrite_port_D(points->y, n_points, dig_fp))
	return -1;

    if (Map->head.with_z) {
	if (0 >= dig__fwrite_port_D(points->z, n_points, dig_fp))
	    return -1;
    }

    if (0 != dig_fflush(dig_fp))
	return -1;

    return offset;
}

/*!
  \brief Deletes feature at the given offset (level 1)
  
  \param Map pointer Map_info structure
  \param offset feature offset
  
  \return  0 on success
  \return -1 on error
*/
int V1_delete_line_nat(struct Map_info *Map, off_t offset)
{
    char rhead;
    struct gvfile *dig_fp;

    G_debug(3, "V1_delete_line_nat(), offset = %lu", (unsigned long) offset);

    dig_set_cur_port(&(Map->head.port));
    dig_fp = &(Map->dig_fp);

    if (dig_fseek(dig_fp, offset, 0) == -1)
	return -1;

    /* read old */
    if (0 >= dig__fread_port_C(&rhead, 1, dig_fp))
	return (-1);

    rhead &= 0xFE;

    if (dig_fseek(dig_fp, offset, 0) == -1)
	return -1;

    if (0 >= dig__fwrite_port_C(&rhead, 1, dig_fp))
	return -1;

    if (0 != dig_fflush(dig_fp))
	return -1;

    return 0;
}

/*!
  \brief Deletes feature at topological level (internal use only)
  
  \param pointer to Map_info structure
  \param line feature id
  
  \return 0 on success
  \return -1 on error
*/
int V2_delete_line_nat(struct Map_info *Map, int line)
{
    int ret, i, side, type, first, next_line, area;
    struct P_line *Line;
    struct P_area *Area;
    struct Plus_head *plus;
    struct bound_box box, abox;
    int adjacent[4], n_adjacent;
    static struct line_cats *Cats = NULL;
    static struct line_pnts *Points = NULL;

    G_debug(3, "V2_delete_line_nat(), line = %d", line);

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
    if (!Points) {
	Points = Vect_new_line_struct();
    }

    type = V2_read_line_nat(Map, Points, Cats, line);

    /* Update category index */
    if (plus->update_cidx) {
	for (i = 0; i < Cats->n_cats; i++) {
	    dig_cidx_del_cat(plus, Cats->field[i], Cats->cat[i], line, type);
	}
    }

    /* delete the line from coor */
    ret = V1_delete_line_nat(Map, Line->offset);

    if (ret == -1) {
	return ret;
    }

    /* Update topology */
    if (plus->built >= GV_BUILD_AREAS && type == GV_BOUNDARY) {
	struct P_topo_b *topo = (struct P_topo_b *)Line->topo;

	/* Store adjacent boundaries at nodes (will be used to rebuild area/isle) */
	/* Adjacent are stored: > 0 - we want right side; < 0 - we want left side */
	n_adjacent = 0;

	next_line = dig_angle_next_line(plus, line, GV_RIGHT, GV_BOUNDARY, NULL);
	if (next_line != 0 && abs(next_line) != line) {
	    /* N1, to the right -> we want the right side for > 0  and left for < 0 */
	    adjacent[n_adjacent] = next_line;
	    n_adjacent++;
	}
	next_line = dig_angle_next_line(plus, line, GV_LEFT, GV_BOUNDARY, NULL);
	if (next_line != 0 && abs(next_line) != line) {
	    /* N1, to the left -> we want the left side for > 0  and right for < 0 */
	    adjacent[n_adjacent] = -next_line;
	    n_adjacent++;
	}
	next_line = dig_angle_next_line(plus, -line, GV_RIGHT, GV_BOUNDARY, NULL);
	if (next_line != 0 && abs(next_line) != line) {
	    /* N2, to the right -> we want the right side for > 0  and left for < 0 */
	    adjacent[n_adjacent] = next_line;
	    n_adjacent++;
	}
	next_line = dig_angle_next_line(plus, -line, GV_LEFT, GV_BOUNDARY, NULL);
	if (next_line != 0 && abs(next_line) != line) {
	    /* N2, to the left -> we want the left side for > 0  and right for < 0 */
	    adjacent[n_adjacent] = -next_line;
	    n_adjacent++;
	}

	/* Delete area(s) and islands this line forms */
	first = 1;
	if (topo->left > 0) {	/* delete area */
	    Vect_get_area_box(Map, topo->left, &box);
	    if (first) {
		Vect_box_copy(&abox, &box);
		first = 0;
	    }
	    else
		Vect_box_extend(&abox, &box);

	    if (plus->update_cidx) {
		V2__delete_area_cats_from_cidx_nat(Map, topo->left);
	    }
	    dig_del_area(plus, topo->left);
	}
	else if (topo->left < 0) {	/* delete isle */
	    dig_del_isle(plus, -topo->left);
	}
	if (topo->right > 0) {	/* delete area */
	    Vect_get_area_box(Map, topo->right, &box);
	    if (first) {
		Vect_box_copy(&abox, &box);
		first = 0;
	    }
	    else
		Vect_box_extend(&abox, &box);

	    if (plus->update_cidx) {
		V2__delete_area_cats_from_cidx_nat(Map, topo->right);
	    }
	    dig_del_area(plus, topo->right);
	}
	else if (topo->right < 0) {	/* delete isle */
	    dig_del_isle(plus, -topo->right);
	}
    }

    /* Delete reference from area */
    if (plus->built >= GV_BUILD_CENTROIDS && type == GV_CENTROID) {
	struct P_topo_c *topo = (struct P_topo_c *)Line->topo;

	if (topo->area > 0) {
	    G_debug(3, "Remove centroid %d from area %d", (int) line, topo->area);
	    if (plus->update_cidx) {
		V2__delete_area_cats_from_cidx_nat(Map, topo->area);
	    }
	    Area = Map->plus.Area[topo->area];
	    if (Area) 
		Area->centroid = 0;
	    else
		G_warning(_("Attempt to access dead area (%d)"), topo->area);
	}
    }

    /* delete the line from topo */
    dig_del_line(plus, line, Points->x[0], Points->y[0], Points->z[0]);

    /* Rebuild areas/isles and attach centroids and isles */
    if (plus->built >= GV_BUILD_AREAS && type == GV_BOUNDARY) {
	int new_areas[4], nnew_areas = 0;

	/* Rebuild areas/isles */
	for (i = 0; i < n_adjacent; i++) {
	    side = (adjacent[i] > 0 ? GV_RIGHT : GV_LEFT);

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
	    /* Reattach isles */
	    if (plus->built >= GV_BUILD_ATTACH_ISLES)
		Vect_attach_isles(Map, &abox);

	    /* Reattach centroids */
	    if (plus->built >= GV_BUILD_CENTROIDS)
		Vect_attach_centroids(Map, &abox);
	}

	if (plus->update_cidx) {
	    for (i = 0; i < nnew_areas; i++) {
		V2__add_area_cats_to_cidx_nat(Map, new_areas[i]);
	    }
	}
    }

    G_debug(3, "updated lines : %d , updated nodes : %d", plus->uplist.n_uplines,
	    plus->uplist.n_upnodes);
    
    return ret;
}

/*!
  \brief Restores feature at the given offset.
  
  \param Map pointer to Map_info structure
  \param offset feature offset
  
  \return  0 on success
  \return -1 on error
*/
int V1_restore_line_nat(struct Map_info *Map, off_t offset)
{
    char rhead;
    struct gvfile *dig_fp;
    
    G_debug(3, "V1_restore_line_nat(), offset = %lu", (unsigned long) offset);
    
    dig_set_cur_port(&(Map->head.port));
    dig_fp = &(Map->dig_fp);
    
    if (dig_fseek(dig_fp, offset, 0) == -1)
	return -1;
    
    /* read old */
    if (0 >= dig__fread_port_C(&rhead, 1, dig_fp))
	return (-1);

    /* mark as alive */
    rhead |= 1;
    
    /* write new */
    if (dig_fseek(dig_fp, offset, 0) == -1)
	return -1;

    if (0 >= dig__fwrite_port_C(&rhead, 1, dig_fp))
	return -1;
    
    if (0 != dig_fflush(dig_fp))
	return -1;
    
    return 0;
}

/*!
  \brief Restores feature at topological level (internal use only)
  
  \param Map pointer to Map_info structure
  \param line feature id
  \param offset feature offset
  
  \return 0 on success
  \return -1 on error
*/
int V2_restore_line_nat(struct Map_info *Map, int line, off_t offset)
{
    int i, ret, type;
    struct P_line *Line;
    struct Plus_head *plus;
    struct bound_box box;
    
    static struct line_pnts *points = NULL;
    static struct line_cats *cats = NULL;
    
    Line = NULL;
    type = 0;
    
    G_debug(3, "V2_restore_line_nat(), line = %d", line);

    plus = &(Map->plus);

    if (plus->built >= GV_BUILD_BASE) {
	Line = Map->plus.Line[line];

	if (Line != NULL)
	    G_fatal_error(_("Attempt to restore alive feature"));
    }

    if (!points) {
	points = Vect_new_line_struct();
    }

    if (!cats) {
	cats = Vect_new_cats_struct();
    }

    /* restore the line in coor */
    ret = V1_restore_line_nat(Map, offset);

    if (ret == -1) {
	return ret;
    }
    
    /* read feature geometry */
    type = V1_read_line_nat(Map, points, cats, offset);
    if (type < 0) {
	return -1;
    }

    /* update category index */
    if (plus->update_cidx) {
	for (i = 0; i < cats->n_cats; i++) {
	    dig_cidx_add_cat(plus, cats->field[i], cats->cat[i], line, type);
	}
    }
    
    /* restore the line from topo */		   
    if (plus->built >= GV_BUILD_BASE) {
	dig_line_box(points, &box);
	dig_restore_line(plus, line, type, points, &box, offset);
	G_debug(3, "  line restored in topo with id = %d", line);
	Vect_box_extend(&(plus->box), &box);
    }
    
    V2__add_line_to_topo_nat(Map, line, points, cats, NULL);

    G_debug(3, "updated lines : %d , updated nodes : %d", plus->uplist.n_uplines,
	    plus->uplist.n_upnodes);

    return ret;
}
