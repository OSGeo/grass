/*!
   \file lib/vector/Vlib/write_sfa.c

   \brief Vector library - write vector feature - simple feature access (level 2)

   Higher level functions for reading/writing/manipulating vectors.

   See write_ogr.c (OGR interface) and write_pg.c (PostGIS interface)
   for imlementation issues.

   \todo SFA version of V2__delete_area_cats_from_cidx_nat()
   \todo function to delete corresponding entry in fidx
   \todo SFA version of V2__add_area_cats_to_cidx_nat
   \todo SFA version of V2__add_line_to_topo_nat

   (C) 2011-2012 by Martin Landa, and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
 */

#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"
#endif

#if defined HAVE_OGR || defined HAVE_POSTGRES
static void V2__add_line_to_topo_sfa(struct Map_info *, int, const struct line_pnts *,
                                     const struct line_cats *);
#endif

/*!
  \brief Writes feature on level 2 (OGR/PostGIS interface, pseudo-topological level)

  \param Map pointer to Map_info structure
  \param type feature type (see V1_write_line_ogr() for list of supported types)
  \param points pointer to line_pnts structure (feature geometry) 
  \param cats pointer to line_cats structure (feature categories)
  
  \return feature index in offset array (related to pseudo-topology)
  \return -1 on error
*/
off_t V2_write_line_sfa(struct Map_info *Map, int type,
			const struct line_pnts *points, const struct line_cats *cats)
{
#if defined HAVE_OGR || defined HAVE_POSTGRES
    int line;
    off_t offset;
    struct Plus_head *plus;
    struct bound_box box;
    struct Format_info_offset *offset_info;
    
    line = 0;
    plus = &(Map->plus);
    
    G_debug(3, "V2_write_line_sfa(): type = %d (format = %d)",
	    type, Map->format);
    
    if (Map->format == GV_FORMAT_POSTGIS) {
	offset_info = &(Map->fInfo.pg.offset);
	offset = V1_write_line_pg(Map, type, points, cats);
    }
    else {
	offset_info = &(Map->fInfo.pg.offset);
	offset = V1_write_line_ogr(Map, type, points, cats);
    }
    if (offset < 0)
	return -1;
    
    /* Update topology */
    if (plus->built >= GV_BUILD_BASE) {
	dig_line_box(points, &box);
	line = dig_add_line(plus, type, points, &box, offset);
	G_debug(3, "\tline added to topo with line = %d", line);
	if (line == 1)
	    Vect_box_copy(&(plus->box), &box);
	else
	    Vect_box_extend(&(plus->box), &box);

	if (type == GV_BOUNDARY) {
	    int ret, cline;
	    long fid;
	    double x, y;
	    
	    struct bound_box box;
	    struct line_pnts *CPoints;

	    /* add virtual centroid to pseudo-topology */
	    ret = Vect_get_point_in_poly(points, &x, &y);
	    if (ret == 0) {
		CPoints = Vect_new_line_struct();
		Vect_append_point(CPoints, x, y, 0.0);
		
		fid = offset_info->array[offset];

		dig_line_box(CPoints, &box);
		cline = dig_add_line(plus, GV_CENTROID,
				     CPoints, &box, fid);
		G_debug(4, "\tCentroid: x = %f, y = %f, cat = %lu, line = %d",
			x, y, fid, cline);	  
		dig_cidx_add_cat(plus, 1, (int) fid,
				 cline, GV_CENTROID);
		
		Vect_destroy_line_struct(CPoints);
	    }
	    else {
		G_warning(_("Unable to calculate centroid for area"));
	    }
	}
	V2__add_line_to_topo_sfa(Map, line, points, cats);
    }


    G_debug(3, "updated lines : %d , updated nodes : %d", plus->uplist.n_uplines,
	    plus->uplist.n_upnodes);

    /* returns int line, but is defined as off_t for compatibility with
     * Write_line_array in write.c */
    return line;
#else
    G_fatal_error(_("GRASS is not compiled with OGR/PostgreSQL support"));
    return -1;
#endif
}

/*!
  \brief Rewrites feature at the given offset on level 2 (OGR/PostGIS
  interface, pseudo-topological level)

  Note: Topology must be built at level >= GV_BUILD_BASE

  \param Map pointer to Map_info structure
  \param line feature id to be rewritten
  \param type feature type (see V1_write_line_ogr() for supported types)
  \param offset unused (kept for consistency)
  \param points pointer to line_pnts structure (feature geometry)
  \param cats pointer to line_cats structure feature categories
  
  \return offset where line was rewritten
  \return -1 on error
*/
off_t V2_rewrite_line_sfa(struct Map_info *Map, int line, int type, off_t offset,
			  const struct line_pnts *points, const struct line_cats *cats)
{
    G_debug(3, "V2_rewrite_line_sfa(): line=%d type=%d offset=%"PRI_OFF_T,
	    line, type, offset);

    if (line < 1 || line > Map->plus.n_lines) {
        G_warning(_("Attempt to access feature with invalid id (%d)"), line);
        return -1;
    }

#if defined HAVE_OGR || defined HAVE_POSTGRES
    if (type != V2_read_line_sfa(Map, NULL, NULL, line)) {
	G_warning(_("Unable to rewrite feature (incompatible feature types)"));
	return -1;
    }

    if (V2_delete_line_sfa(Map, line) != 0)
        return -1;

    return V2_write_line_sfa(Map, type, points, cats);
#else
    G_fatal_error(_("GRASS is not compiled with OGR/PostgreSQL support"));
    return -1;
#endif
}

/*!
  \brief Deletes feature on level 2 (OGR/PostGIS interface)

  Note: Topology must be built at level >= GV_BUILD_BASE
  
  \todo Update fidx
  
  \param pointer to Map_info structure
  \param line feature id to be deleted
  
  \return 0 on success
  \return -1 on error
*/
int V2_delete_line_sfa(struct Map_info *Map, int line)
{
#if defined HAVE_OGR || defined HAVE_POSTGRES
    int ret, i, type, first;
    struct P_line *Line;
    struct Plus_head *plus;
    static struct line_cats *Cats = NULL;
    static struct line_pnts *Points = NULL;

    G_debug(3, "V2_delete_line_sfa(): line = %d", line);

    type = first = 0;
    Line = NULL;
    plus = &(Map->plus);

    if (line < 1 || line > Map->plus.n_lines) {
        G_warning(_("Attempt to access feature with invalid id (%d)"), line);
        return -1;
    }

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

    type = V2_read_line_sfa(Map, Points, Cats, line);
    if (type < 0)
        return -1;
    
    /* Update category index */
    if (plus->update_cidx) {
      for (i = 0; i < Cats->n_cats; i++) {
	    dig_cidx_del_cat(plus, Cats->field[i], Cats->cat[i], line, type);
	}
    }
    /* Update fidx */
    /* TODO */
    
    /* delete the line from coor */
    if (Map->format == GV_FORMAT_POSTGIS)
	ret = V1_delete_line_pg(Map, Line->offset);
    else
	ret = V1_delete_line_ogr(Map, Line->offset);
    
    if (ret == -1) {
	return ret;
    }

    /* Update topology */
    if (plus->built >= GV_BUILD_AREAS && type == GV_BOUNDARY) {
	/* TODO */
	/* remove centroid together with boundary (is really an OGR polygon) */
    }
    /* Delete reference from area */
    if (plus->built >= GV_BUILD_CENTROIDS && type == GV_CENTROID) {
	/* for OGR mapsets, virtual centroid will be removed when
	 * polygon is removed */
    }

    /* delete the line from topo */
    dig_del_line(plus, line, Points->x[0], Points->y[0], Points->z[0]);

    /* Rebuild areas/isles and attach centroids and isles */
    if (plus->built >= GV_BUILD_AREAS && type == GV_BOUNDARY) {
	/* maybe not needed VERIFY */
    }
    return ret;
#else
    G_fatal_error(_("GRASS is not compiled with OGR/PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Writes area on topological level (Simple Features interface,
   internal use only)

   \param Map pointer to Map_info structure
   \param points feature geometry (exterior + interior rings)
   \param nparts number of parts including exterior ring
   \param cats feature categories
   
   \return feature offset
   \return -1 on error
*/
off_t V2__write_area_sfa(struct Map_info *Map, 
                         const struct line_pnts **points, int nparts,
                         const struct line_cats *cats)
{
    if (Map->format == GV_FORMAT_OGR) {
#ifdef HAVE_OGR
        return V2__write_area_ogr(Map, points, nparts, cats);
#else
        G_fatal_error(_("GRASS is not compiled with OGR support"));
        return -1;
#endif
    }
    else if (Map->format == GV_FORMAT_POSTGIS) {
#ifdef HAVE_POSTGRES
        return V2__write_area_pg(Map, points, nparts, cats);
#else
        G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
        return -1;
#endif
    }
    else {
        G_warning(_("Unsupported vector map format (%d)"), Map->format);
    }
    return -1;
}

#if defined HAVE_OGR || defined HAVE_POSTGRES
/*!
  \brief Add feature to topo file (internal use only)

  \param Map pointer to Map_info structure
  \param line feature id
  \param points pointer to line_pnts structure (feature geometry)
  \param cats pointer to line_cats structure (feature categories)
*/
void V2__add_line_to_topo_sfa(struct Map_info *Map, int line,
			      const struct line_pnts *points,
			      const struct line_cats *cats)
{
    int first, s, i;
    int type, area, side;

    struct Plus_head *plus;
    struct P_line *Line;
    
    struct bound_box box, abox;
    
    G_debug(3, "V2__add_line_to_topo_sfa(): line = %d npoints = %d", line,
	    points->n_points);

    plus = &(Map->plus);
    Line = plus->Line[line];
    type = Line->type;

    if (plus->built >= GV_BUILD_AREAS &&
	type == GV_BOUNDARY) {	
	struct P_topo_b *topo = (struct P_topo_b *)Line->topo;
	
	if (topo->N1 != topo->N2) {
	    G_warning(_("Boundary is not closed. Skipping."));
	    return;
	}
	
	/* Build new areas/isles */
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
	    G_debug(4, "Vect_build_line_area(): -> area = %d", area);
	}

	/* Attach centroid/isle to the new area */
	if (plus->built >= GV_BUILD_ATTACH_ISLES)
	    Vect_attach_isles(Map, &abox);
	if (plus->built >= GV_BUILD_CENTROIDS)
	    Vect_attach_centroids(Map, &abox);
    }
    
    /* Add category index */
    for (i = 0; i < cats->n_cats; i++) {
	dig_cidx_add_cat_sorted(plus, cats->field[i], cats->cat[i], line,
				type);
    }
    
    return;
}
#endif /* HAVE_OGR || HAVE_POSTGRES */
