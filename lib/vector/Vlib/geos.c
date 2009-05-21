/*!
  \file geos.c
  
  \brief Vector library - GEOS support
  
  Higher level functions for reading/writing/manipulating vectors.
  
  (C) 2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
 */

#include <grass/config.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

#ifdef HAVE_GEOS
#include <geos_c.h>

static GEOSGeometry *Vect__read_line_geos(struct Map_info *, long, int *);
static GEOSCoordSequence *V1_read_line_geos(struct Map_info *, long, int *);
static GEOSCoordSequence *V2_read_line_geos(struct Map_info *, int);

/*!
   \brief Read vector feature and stores it as GEOSGeometry instance

   Supported feature types:
    - GV_POINT     -> POINT
    - GV_LINE      -> LINESTRING
    - GV_BOUNDARY  -> LINESTRING / LINEARRING
   
   You should free allocated memory by GEOSGeom_destroy().

   \param Map pointer to Map_info structure
   \param line feature id
   \param[out] type feature type or NULL
   
   \return pointer to GEOSGeometry instance
   \return empty GEOSGeometry for unsupported feature type
   \return NULL on error
 */
GEOSGeometry *Vect_read_line_geos(struct Map_info *Map, int line, int *type)
{
    P_LINE *Line;
    
    G_debug(3, "Vect_read_line_geos(): line = %d", line);
    
    if (!VECT_OPEN(Map))
	G_fatal_error("Vect_read_line_geos(): %s", _("vector map is not opened"));
    
    if (line < 1 || line > Map->plus.n_lines)
	G_fatal_error(_("Vect_read_line_geos(): feature id %d is not reasonable "
			"(max features in vector map <%s>: %d)"),
		      line, Vect_get_full_name(Map), Map->plus.n_lines);
    
    if (Map->format != GV_FORMAT_NATIVE)
	G_fatal_error("Vect_read_line_geos(): %s", _("only native format supported"));
    
    Line = Map->plus.Line[line];
    if (Line == NULL)
	G_fatal_error("Vect_read_line_geos(): %s %d",
		      _("Attempt to read dead line"), line);
    
    return Vect__read_line_geos(Map, Line->offset, type);
}

/*!
   \brief Read vector area and stores it as GEOSGeometry instance (polygon)

   You should free allocated memory by GEOSGeom_destroy().

   \param Map pointer to Map_info structure
   \param area area id 

   \return pointer to GEOSGeometry instance
   \return NULL on error
 */
GEOSGeometry *Vect_read_area_geos(struct Map_info * Map, int area)
{
    int i, nholes, isle;
    GEOSGeometry *boundary, **holes;
    
    G_debug(3, "Vect_read_area_geos(): area = %d", area);

    boundary = GEOSGeom_createLinearRing(Vect_get_area_points_geos(Map, area));
    if (!boundary) {
	G_fatal_error(_("Vect_read_area_geos(): unable to read area id %d"),
		      area);
    }

    nholes = Vect_get_area_num_isles(Map, area);
    holes = (GEOSGeometry **) G_malloc(nholes * sizeof(GEOSGeometry *));
    for (i = 0; i < nholes; i++) {
	isle = Vect_get_area_isle(Map, area, i);
	if (isle < 1)
	    continue;
	holes[i] = GEOSGeom_createLinearRing(Vect_get_isle_points_geos(Map, isle));
	if (!(holes[i]))
	    G_fatal_error(_("Vect_read_area_geos(): unable to read isle id %d of area id %d"),
			  isle, area);
    }
    
    return GEOSGeom_createPolygon(boundary, holes, nholes);
}

/*!
   \brief Create GEOSGeometry of given type from feature points.

   Supported types:
   - GV_POINT    -> POINT
   - GV_LINE     -> LINESTRING
   - GV_BOUNDARY -> LINEARRING

   You should free allocated memory by GEOSGeom_destroy().

   \param Map pointer to Map_info structure
   \param type feature type (see supported types)

   \return pointer to GEOSGeometry instance
   \return NULL on error
 */
GEOSGeometry *Vect_line_to_geos(struct Map_info *Map,
				const struct line_pnts *points, int type)
{
    int i, with_z;
    GEOSGeometry *geom;
    GEOSCoordSequence *pseq;

    G_debug(3, "Vect_line_to_geos(): type = %d", type);
    
    with_z = Vect_is_3d(Map);
    
    /* read only points / lines / boundaries */
    if (!(type & (GV_POINT | GV_LINES)))
	return NULL;

    if (type == GV_POINT) { 
	if (points->n_points != 1)
	    /* point is not valid */
	    return NULL;
    }
    else {			
	if (points->n_points < 2)
	    /* line/boundary is not valid */
	    return NULL;
    }
    
    pseq = GEOSCoordSeq_create(points->n_points, with_z ? 3 : 2);
    
    for (i = 0; i < points->n_points; i++) {
	GEOSCoordSeq_setX(pseq, i, points->x[i]);
	GEOSCoordSeq_setY(pseq, i, points->y[i]);
	if (with_z)
	    GEOSCoordSeq_setZ(pseq, i, points->z[i]);
    }

    if (type == GV_POINT)
	geom = GEOSGeom_createPoint(pseq);
    else if (type == GV_LINE)
	geom = GEOSGeom_createLineString(pseq);
    else { /* boundary */
	geom = GEOSGeom_createLineString(pseq);
	if (GEOSisRing(geom)) {
	    /* GEOSGeom_destroy(geom); */
	    geom = GEOSGeom_createLinearRing(pseq);
	}
    }
    
    /* GEOSCoordSeq_destroy(pseq); */

    return geom;
}

/*!  
  \brief Read line from coor file
  
  You should free allocated memory by GEOSGeom_destroy().

  \param Map pointer to Map_info
  \param offset line offset
  \param[out] type feature type or NULL

  \return pointer to GEOSGeometry
  \return NULL on error
  \return NULL dead line
  \return NULL end of file
*/
GEOSGeometry *Vect__read_line_geos(struct Map_info *Map, long offset, int *type)
{
    int ftype;
    double *x, *y, *z;
    
    GEOSGeometry *geom;
    GEOSCoordSequence *pseq;
    
    pseq = V1_read_line_geos(Map, offset, &ftype);
    if (!pseq)
	G_fatal_error(_("Unable to read line offset %ld"), offset);
    
    if (ftype & GV_POINT) {
	G_debug(3, "    geos_type = point");
	geom = GEOSGeom_createPoint(pseq);
    }
    else if (ftype & GV_LINE) {
	G_debug(3, "    geos_type = linestring");
	geom = GEOSGeom_createLineString(pseq);
    }
    else { /* boundary */
	geom = GEOSGeom_createLineString(pseq);
	if (GEOSisRing(geom)) {
	    /* GEOSGeom_destroy(geom); */
	    geom = GEOSGeom_createLinearRing(pseq);
	    G_debug(3, "    geos_type = linearring");
	}
	else {
	    G_debug(3, "    geos_type = linestring");
	}
    }
    
    G_free((void *) x);
    G_free((void *) y);
    if (z)
	G_free((void *) z);
    
    /* GEOSCoordSeq_destroy(pseq); */
    
    if (type)
      *type = ftype;
    
    return geom;
}

/*!  
  \brief Read line from coor file into GEOSCoordSequence
  
  You should free allocated memory by GEOSCoordSeq_destroy().
  
  \param Map pointer to Map_info
  \param line line id
  
  \return pointer to GEOSCoordSequence
  \return empty GEOSCoordSequence for dead line or unsuppored feature type
  \return NULL end of file
*/
GEOSCoordSequence *V2_read_line_geos(struct Map_info *Map, int line)
{
    int ftype;
    P_LINE *Line;
    
    G_debug(3, "V2_read_line_geos(): line = %d", line);
    
    Line = Map->plus.Line[line];

    if (Line == NULL)
	G_fatal_error("V2_read_line_geos(): %s %d",
		      _("Attempt to read dead line"), line);
    
    return V1_read_line_geos(Map, Line->offset, &ftype);
}


/*!  
  \brief Read feature from coor file into GEOSCoordSequence

  Note: Function reads only points, lines and boundaries, other
  feature types are ignored (empty coord array is returned)!
  
  You should free allocated memory by GEOSCoordSeq_destroy().
  
  \param Map pointer to Map_info
  \param offset line offset
  \param[out] type feature type
  
  \return pointer to GEOSCoordSequence
  \return empty GEOSCoordSequence for dead line or unsuppored feature type
  \return NULL end of file
*/
GEOSCoordSequence *V1_read_line_geos(struct Map_info *Map, long offset, int *type)
{
    int i, n_points;
    char rhead;
    double *x, *y, *z;
    
    GEOSCoordSequence *pseq;
    
    G_debug(3, "V1_read_line_geos(): offset = %ld", offset);
    
    Map->head.last_offset = offset;
    
    /* reads must set in_head, but writes use default */
    dig_set_cur_port(&(Map->head.port));
    
    dig_fseek(&(Map->dig_fp), offset, 0);
    
    if (0 >= dig__fread_port_C(&rhead, 1, &(Map->dig_fp)))
	return NULL;            /* end of file */
    
    if (!(rhead & 0x01))	/* dead line */
	return GEOSCoordSeq_create(0, (Map->head.with_z) ? 3 : 2);
    
    rhead >>= 2;
    *type = dig_type_from_store((int) rhead);
    
    /* read only points / lines / boundaries */
    if (!(*type & (GV_POINT | GV_LINES)))
	return GEOSCoordSeq_create(0, (Map->head.with_z) ? 3 : 2);
    
    if (*type & GV_POINTS) {
	n_points = 1;
    }
    else {
	if (0 >= dig__fread_port_I(&n_points, 1, &(Map->dig_fp)))
	    return NULL;
    }
    
    G_debug(3, "    n_points = %d dim = %d", n_points, (Map->head.with_z) ? 3 : 2);
    
    pseq = GEOSCoordSeq_create(n_points, (Map->head.with_z) ? 3 : 2);
    
    x = (double *) G_malloc(n_points * sizeof(double));
    y = (double *) G_malloc(n_points * sizeof(double));
    if (Map->head.with_z)
	z = (double *) G_malloc(n_points * sizeof(double));
    else
	z = NULL;
    
    if (0 >= dig__fread_port_D(x, n_points, &(Map->dig_fp)))
	return NULL; /* end of file */

    if (0 >= dig__fread_port_D(y, n_points, &(Map->dig_fp)))
	return NULL; /* end of file */

    if (Map->head.with_z) {
	if (0 >= dig__fread_port_D(z, n_points, &(Map->dig_fp)))
	    return NULL; /* end of file */

    }

    for (i = 0; i < n_points; i++) {
	GEOSCoordSeq_setX(pseq, i, x[i]);
	GEOSCoordSeq_setY(pseq, i, y[i]);
	if (Map->head.with_z)
	    GEOSCoordSeq_setZ(pseq, i, z[i]);
    }
    
    G_debug(3, "    off = %ld", dig_ftell(&(Map->dig_fp)));
    
    G_free((void *) x);
    G_free((void *) y);
    if (z)
	G_free((void *) z);
    
    return pseq;
}

/*!
   \brief Returns the polygon array of points, i.e. outer ring (shell)

   You should free allocated memory by GEOSCoordSeq_destroy().

   See also Vect_get_area_points().

   \param Map pointer to Map_info
   \param area area id

   \return pointer to GEOSCoordSequence
   \return empty GEOSCoordSequence for dead area
   \return NULL on error
 */
GEOSCoordSequence *Vect_get_area_points_geos(struct Map_info *Map, int area)
{
    int i, j, k;
    int line, aline;
    unsigned int n_points, n_points_shell;
    double x, y, z;

    struct Plus_head *Plus;
    P_AREA *Area;
    
    GEOSCoordSequence **pseq, *pseq_shell;
    
    G_debug(3, "Vect_get_area_points_geos(): area = %d", area);
    
    Plus = &(Map->plus);
    Area = Plus->Area[area];

    if (Area == NULL) {		/* dead area */
	G_warning(_("Attempt to read points of nonexistent area id %d"), area);
	return NULL;		/* error , because we should not read dead areas */
    }
    
    G_debug(3, "  n_lines = %d", Area->n_lines);
    pseq = (GEOSCoordSequence **) G_malloc(Area->n_lines * sizeof(GEOSCoordSequence *));

    n_points_shell = 0;
    for (i = 0; i < Area->n_lines; i++) {
	line = Area->lines[i];
	aline = abs(line);
	G_debug(3, "  append line(%d) = %d", i, line);

	/*
	  TODO
	if (line > 0)
	    dir = GV_FORWARD;
	else
	    dir = GV_BACKWARD;
	*/

	pseq[i] = V2_read_line_geos(Map, aline);
	if (!(pseq[i])) {
	    G_fatal_error(_("Unable to read feature id %d"), aline);
	}
	
	GEOSCoordSeq_getSize(pseq[i], &n_points);
	G_debug(3, "  line n_points = %d", n_points);
	n_points_shell += n_points;
    }

    /* create shell (outer ring) */
    pseq_shell = GEOSCoordSeq_create(n_points_shell, Map->head.with_z ? 3 : 2);
    k = 0;
    for (i = 0; i < Area->n_lines; i++) {
	GEOSCoordSeq_getSize(pseq[i], &n_points);
	for (j = 0; j < (int) n_points; j++, k++) {
	    GEOSCoordSeq_getX(pseq[i], j, &x);
	    GEOSCoordSeq_setX(pseq_shell, k, x);
	    GEOSCoordSeq_getY(pseq[i], j, &y);
	    GEOSCoordSeq_setY(pseq_shell, k, y);
	    if (Map->head.with_z) {
		GEOSCoordSeq_getY(pseq[i], j, &z);
		GEOSCoordSeq_setZ(pseq_shell, k, z);
	    }
	}
    }
    
    G_free((void *) pseq);

    return pseq_shell;
}

/*!
   \brief Returns the polygon (isle) array of points (inner ring)

   You should free allocated memory by GEOSCoordSeq_destroy().
   
   See also Vect_get_isle_points().

   \param Map pointer to Map_info
   \param isle isel id

   \return pointer to GEOSGeometry
   \return NULL on error or dead line
 */
GEOSCoordSequence *Vect_get_isle_points_geos(struct Map_info *Map, int isle)
{
    int i, j, k;
    int line, aline;
    unsigned n_points, n_points_shell;
    double x, y, z;

    struct Plus_head *Plus;
    P_ISLE *Isle;

    GEOSCoordSequence **pseq, *pseq_shell;

    G_debug(3, "Vect_get_isle_points_geos(): isle = %d", isle);

    Plus = &(Map->plus);
    Isle = Plus->Isle[isle];
    
    G_debug(3, "  n_lines = %d", Isle->n_lines);
    for (i = 0; i < Isle->n_lines; i++) {
	line = Isle->lines[i];
	aline = abs(line);
	G_debug(3, "  append line(%d) = %d", i, line);


	pseq[i] = V2_read_line_geos(Map, aline);
	if (!(pseq[i])) {
	    G_fatal_error(_("Unable to read feature id %d"), aline);
	}

	GEOSCoordSeq_getSize(pseq[i], &n_points);
	G_debug(3, "  line n_points = %d", n_points);
	n_points_shell += n_points;

	/*
	  TODO
	if (line > 0)
	    dir = GV_FORWARD;
	else
	    dir = GV_BACKWARD;
	*/
    }

    /* create shell (outer ring) */
    pseq_shell = GEOSCoordSeq_create(n_points_shell, Map->head.with_z ? 3 : 2);
    k = 0;
    for (i = 0; i < Isle->n_lines; i++) {
	GEOSCoordSeq_getSize(pseq[i], &n_points);
	for (j = 0; j < (int) n_points; j++, k++) {
	    GEOSCoordSeq_getX(pseq[i], j, &x);
	    GEOSCoordSeq_setX(pseq_shell, k, x);
	    GEOSCoordSeq_getY(pseq[i], j, &y);
	    GEOSCoordSeq_setY(pseq_shell, k, y);
	    if (Map->head.with_z) {
		GEOSCoordSeq_getY(pseq[i], j, &z);
		GEOSCoordSeq_setZ(pseq_shell, k, z);
	    }
	}
    }
    
    G_free((void *) pseq);

    return pseq_shell;
}

#endif /* HAVE_GEOS */
