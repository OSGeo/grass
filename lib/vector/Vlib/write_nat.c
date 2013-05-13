/*!
   \file lib/vector/Vlib/write_nat.c

   \brief Vector library - write/modify/delete vector feature (native format)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2013 by the GRASS Development Team

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

static struct line_cats *Cats;
static struct line_pnts *Points;

static off_t V1__rewrite_line_nat(struct Map_info *, off_t, int,
				  const struct line_pnts *, const struct line_cats *);
static void V2__delete_area_cats_from_cidx_nat(struct Map_info *, int);
static void V2__add_area_cats_to_cidx_nat(struct Map_info *, int);

/*!
  \brief Writes feature to 'coor' file at level 1 (internal use only)
  
  \param Map pointer to Map_info structure
  \param type feature type (GV_POINT, GV_LINE, ...)
  \param points feature geometry
  \param cats feature categories
  
  \return feature offset into file
  \return -1 on error
*/
off_t V1_write_line_nat(struct Map_info *Map, int type,
                        const struct line_pnts *points, const struct line_cats *cats)
{
    off_t offset;

    if (dig_fseek(&(Map->dig_fp), 0L, SEEK_END) == -1)	/* set to end of file */
	return -1;

    offset = dig_ftell(&(Map->dig_fp));
    G_debug(3, "V1_write_line_nat(): offset = %lu", offset);
    if (offset == -1)
	return -1;

    return V1__rewrite_line_nat(Map, offset, type, points, cats);
}

/*!
  \brief Writes feature to 'coor' file at topological level (internal use only)

  Note: Function returns feature id, but is defined as off_t for
  compatibility with level 1 functions.
  
  \param Map pointer to Map_info structure
  \param type feature type (GV_POINT, GV_LINE, ...)
  \param points feature geometry
  \param cats feature categories
  
  \return new feature id 
  \return 0 topology is not requested to be built (build level < GV_BUILD_BASE)
  \return -1 on error
*/
off_t V2_write_line_nat(struct Map_info *Map, int type,
			const struct line_pnts *points, const struct line_cats *cats)
{
    off_t offset;

    G_debug(3, "V2_write_line_nat(): type=%d", type);
    
    /* write feature to 'coor' file */
    offset = V1_write_line_nat(Map, type, points, cats);
    if (offset < 0)
	return -1;

    /* update topology (build level >= GV_BUILD_BASE) */
    return V2__add_line_to_topo_nat(Map, offset, type, points, cats, -1, NULL);
}

/*!
  \brief Rewrites feature to 'coor' file at level 1 (internal use only)
  
  If the number of points or cats differs from the original one or the
  type is changed: GV_POINTS -> GV_LINES or GV_LINES -> GV_POINTS, the
  old one is deleted and the new is appended to the end of the file.
  
  Old feature is deleted (marked as dead), and a new feature written.
  
  \param Map pointer to Map_info structure
  \param line feature id to be rewritten
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

  Note: Topology must be built at level >= GV_BUILD_BASE
  
  Note: Function returns feature id, but is defined as off_t for
  compatibility with level 1 functions.
  
  \param Map pointer to Map_info structure
  \param type feature type (GV_POINT, GV_LINE, ...)
  \param line feature id to be rewritten
  \param old_offset feature offset
  \param points feature geometry
  \param cats feature categories
  
  \return new feature id
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

    if (0 != V2_delete_line_nat(Map, line))
        return -1;
    
    G_debug(3, "V2_write_line_nat(), line = %d", line);

    /* rewrite feature in 'coor' file */
    offset = V1_rewrite_line_nat(Map, line, type, old_offset, points, cats);
    if (offset < 0)
	return -1;

    /* update topology */
    return V2__add_line_to_topo_nat(Map, offset, type, points, cats, -1, NULL);
}

/*!
  \brief Deletes feature at level 1 (internal use only)
  
  \param Map pointer Map_info structure
  \param offset feature offset
  
  \return  0 on success
  \return -1 on error
*/
int V1_delete_line_nat(struct Map_info *Map, off_t offset)
{
    char rhead;
    struct gvfile *dig_fp;

    G_debug(3, "V1_delete_line_nat(): offset = %lu", (unsigned long) offset);

    dig_set_cur_port(&(Map->head.port));
    dig_fp = &(Map->dig_fp);

    if (dig_fseek(dig_fp, offset, 0) == -1)
	return -1;

    /* read old */
    if (0 >= dig__fread_port_C(&rhead, 1, dig_fp))
	return -1;

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

  Note: Topology must be built at level >= GV_BUILD_BASE  
  
  \param pointer to Map_info structure
  \param line feature id
  
  \return 0 on success
  \return -1 on error
*/
int V2_delete_line_nat(struct Map_info *Map, int line)
{
    int type;
    struct P_line *Line;
    struct Plus_head *plus;

    G_debug(3, "V2_delete_line_nat(): line = %d", line);

    Line = NULL;
    plus = &(Map->plus);

    if (line < 1 || line > plus->n_lines) {
        G_warning(_("Attempt to access feature with invalid id (%d)"), line);
        return -1;
    }
    
    Line = Map->plus.Line[line];
    if (Line == NULL) {
        G_warning(_("Attempt to access dead feature %d"), line);
        return -1;
    }

    /* read the line */    
    if (!Points) {
	Points = Vect_new_line_struct();
    }
    if (!Cats) {
	Cats = Vect_new_cats_struct();
    }
    type = V2_read_line_nat(Map, Points, Cats, line);
    if (type < 0)
        return -1;

    /* delete feature from coor file */
    if (0 != V1_delete_line_nat(Map, Line->offset))
        return -1;

    /* delete feature from topology */
    if (0 != V2__delete_line_from_topo_nat(Map, line, type, Points, Cats))
        return -1;
    
    return 0;
}

/*!
  \brief Restores feature at level 1 (internal use only)
  
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
	return -1;

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

  Note: This function requires build level >= GV_BUILD_BASE.
  
  \param Map pointer to Map_info structure
  \param line feature id
  \param offset feature offset
  
  \return 0 on success
  \return -1 on error
*/
int V2_restore_line_nat(struct Map_info *Map, int line, off_t offset)
{
    int type;
    struct Plus_head *plus;
    struct P_line *Line;
    
    plus = &(Map->plus);

    G_debug(3, "V2_restore_line_nat(), line = %d", line);

    if (line < 1 || line > plus->n_lines) {
        G_warning(_("Attempt to access feature with invalid id (%d)"), line);
        return -1;
    }
    
    Line = Map->plus.Line[line];
    if (Line != NULL) {
        G_warning(_("Attempt to access alive feature %d"), line);
        return -1;
    }
    
    /* restore feature in 'coor' file */
    if (0 != V1_restore_line_nat(Map, offset))
	return -1;


    /* read feature geometry */    
    if (!Points)
	Points = Vect_new_line_struct();
    if (!Cats)
	Cats = Vect_new_cats_struct();
    type = V1_read_line_nat(Map, Points, Cats, offset);
    if (type < 0)
	return -1;

    /* update topology */
    return V2__add_line_to_topo_nat(Map, offset, type, Points, Cats, line, NULL) > 0 ? 0 : -1;
}

/*** static or internal subroutines bellow ****/

/*!
  \brief Rewrites feature at the given offset 
  
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
	if (Map->head.coor_version.minor == 1) {	/* coor format 5.1 */
	    if (0 >= dig__fwrite_port_I(&(cats->n_cats), 1, dig_fp))
		return -1;
	}
	else {			/* coor format 5.0 */
	    nc = (char)cats->n_cats;
	    if (0 >= dig__fwrite_port_C(&nc, 1, dig_fp))
		return -1;
	}

	if (cats->n_cats > 0) {
	    if (Map->head.coor_version.minor == 1) {	/* coor format 5.1 */
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
  \brief Deletes area (i.e. centroid) categories from category
  index (internal use only)

  Call G_fatal_error() when area do not exits.
  
  \param Map pointer to Map_info structure
  \param area area id
*/
void V2__delete_area_cats_from_cidx_nat(struct Map_info *Map, int area)
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
void V2__add_area_cats_to_cidx_nat(struct Map_info *Map, int area)
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
  \brief Delete feature from topology (internal use only)

  Note: This function requires build level >= GV_BUILD_BASE.

  Also updates category index if requested.

  Calls G_warning() on error.
  
  \param Map pointer to Map_info struct
  \param line feature id to be removed
  \param Points feature geometry (pointer to line_pnts struct)
  \param external_routine external subroutine to execute (used by PostGIS Topology)

  \return 0 on success
  \return -1 on failure
 */
int V2__delete_line_from_topo_nat(struct Map_info *Map, int line, int type,
                                  const struct line_pnts *points, const struct line_cats *cats)
{
    int i, first;
    int adjacent[4], n_adjacent;
    
    struct bound_box box, abox;
    struct Plus_head *plus;
    struct P_line *Line;
    
    n_adjacent = 0;
    
    plus = &(Map->plus);
    
    if (line < 1 || line > plus->n_lines) {
        G_warning(_("Attempt to access feature with invalid id (%d)"), line);
        return -1;
    }
    
    Line = Map->plus.Line[line];
    if (!Line) {
        G_warning(_("Attempt to access dead feature %d"), line);
        return -1;
    }

    /* delete feature from category index */
    if (plus->update_cidx && cats) {
        for (i = 0; i < cats->n_cats; i++) {
            dig_cidx_del_cat(plus, cats->field[i], cats->cat[i], line, type);
        }
    }
    
    /* update areas when deleting boundary from topology */
    if (plus->built >= GV_BUILD_AREAS && Line->type == GV_BOUNDARY) {
        int next_line;
        
        struct P_topo_b *topo = (struct P_topo_b *)Line->topo;

	/* store adjacent boundaries at nodes (will be used to rebuild area/isle) */
	/* adjacent are stored: > 0 - we want right side; < 0 - we want left side */
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

	/* delete area(s) and islands this line forms */
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

    /* delete reference from area */
    if (plus->built >= GV_BUILD_CENTROIDS && Line->type == GV_CENTROID) {
        struct P_area *Area;
        
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
		G_warning(_("Attempt to access dead area %d"), topo->area);
	}
    }

    /* delete the line from topo */
    if (0 != dig_del_line(plus, line, points->x[0], points->y[0], points->z[0]))
        return -1;
    
    /* rebuild areas/isles and attach centroids and isles */
    if (plus->built >= GV_BUILD_AREAS && type == GV_BOUNDARY) {
        int i, side, area;
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
	/* reattach all centroids/isles in deleted areas + new area.
	 *  because isles are selected by box it covers also possible new isle created above */
	if (!first) {		/* i.e. old area/isle was deleted or new one created */
	    /* reattach isles */
	    if (plus->built >= GV_BUILD_ATTACH_ISLES)
		Vect_attach_isles(Map, &abox);

	    /* reattach centroids */
	    if (plus->built >= GV_BUILD_CENTROIDS)
		Vect_attach_centroids(Map, &abox);
	}

	if (plus->update_cidx) {
	    for (i = 0; i < nnew_areas; i++) {
		V2__add_area_cats_to_cidx_nat(Map, new_areas[i]);
	    }
	}
    }

    if (plus->uplist.do_uplist) {
        G_debug(3, "updated lines : %d , updated nodes : %d", plus->uplist.n_uplines,
                plus->uplist.n_upnodes);
    }

    return 0;
}

/*!
  \brief Add feature (line) to topology (internal use only)

  Also updates category index if requested.

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
    \param line feature id to be added
    \param points pointer to line_pnts structure (feature's geometry)
    \param cats pointer to line_cats structure (feature's categories)
    \param external_routine pointer to external routine (used by PostGIS Topology)

    \return feature id to be added
    \return 0 nothing to do (build level must be >= GV_BUILD_BASE) 
    \return -1 on error 
*/
int V2__add_line_to_topo_nat(struct Map_info *Map, off_t offset, int type,
                             const struct line_pnts *points, const struct line_cats *cats,
                             int restore_line,
                             int (*external_routine) (const struct Map_info *, int))
{
    int first, s, n, i, line;
    int node, next_line, area, side, sel_area, new_area[2];

    struct Plus_head *plus;
    struct P_line *Line, *NLine;
    struct P_node *Node;
    struct P_area *Area;
    
    struct bound_box box, abox;

    plus = &(Map->plus);
    
    G_debug(3, "V2__add_line_to_topo_nat(): offset = %ld (build level = %d)", offset, plus->built);

    if (plus->built < GV_BUILD_BASE) /* nothing to build */
        return 0;
    
    /* add line to topology */
    dig_line_box(points, &box);
    if (restore_line > 0)
        line = dig_restore_line(plus, restore_line, type, points, &box, offset);
    else
        line = dig_add_line(plus, type, points, &box, offset);
    G_debug(3, "  line added to topo with id = %d", line);
    
    Line = plus->Line[line];
    
    /* extend map bounding box */
    if (line == 1)
        Vect_box_copy(&(plus->box), &box);
    else
        Vect_box_extend(&(plus->box), &box);

    /* build areas on left/right side */
    if (plus->built >= GV_BUILD_AREAS &&
	type == GV_BOUNDARY) {
	struct P_topo_b *topo = (struct P_topo_b *)Line->topo;
	
	/* delete neighbour areas/isles */
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
    
    /* attach centroid */
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

    /* add category index */
    if (plus->update_cidx && cats) {
        for (i = 0; i < cats->n_cats; i++) {
            dig_cidx_add_cat_sorted(plus, cats->field[i], cats->cat[i], line,
                                    type);
        }
    }

    if (plus->uplist.do_uplist) {
        G_debug(3, "updated lines : %d , updated nodes : %d", plus->uplist.n_uplines,
                plus->uplist.n_upnodes);
    }
    
    return line;
}
