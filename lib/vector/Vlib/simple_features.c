/*!
  \file lib/vector/Vlib/simple_features.c
 
  \brief Vector library - OGC Simple Features Access

  Higher level functions for reading/writing/manipulating vectors.
  
  Note: <b>In progress!</b> Currently on GV_POINT, GV_LINE,
  GV_BOUNDARY are supported.
  
  \todo
   - Vect_sfa_line_is_simple()
   - Vect_sfa_line_srid()
   - Vect_sfa_line_envelope()
   - Vect_sfa_line_asbinary()
   - Vect_sfa_line_is_empty()
   - Vect_sfa_line_is_3d()
   - Vect_sfa_line_is_measured()
   - Vect_sfa_line_boundary()

  Reference: http://www.opengeospatial.org/standards/sfa

  (C) 2009, 2011-2013 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <stdio.h>

#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"
#endif

#ifdef HAVE_OGR
#include <ogr_api.h>
#endif

static int check_sftype(const struct line_pnts *, int, SF_FeatureType, int);
static int get_sftype(const struct line_pnts *, int, int);
static void print_point(const struct line_pnts *, int, int, int, FILE *);

/*!
  \brief Get SF type of given vector feature

  List of supported feature types:
   - GV_POINT         -> SF_POINT
   - GV_LINE          -> SF_LINESTRING
   - GV_LINE (closed) -> SF_LINEARRING
   - GV_BOUNDARY      -> SF_POLYGON

  \param Points  pointer to line_pnts structure
  \param type    feature type (see supported types above)
  \param with_z  WITH_Z for 3D data

  \return SF type identificator (see list of supported types)
  \return -1 on error
*/
SF_FeatureType Vect_sfa_get_line_type(const struct line_pnts *Points, int type, int with_z)
{
    return get_sftype(Points, type, with_z);
}

/*!
  \brief Get relevant GV type

  \param Map pointer to Map_info structure
  \param type SF geometry type (SF_POINT, SF_LINESTRING, ...)

  \return GV type
  \return -1 on error
 */
int Vect_sfa_get_type(SF_FeatureType sftype)
{
    switch(sftype) {
    case SF_POINT:
    case SF_POINT25D:
	return GV_POINT;
    case SF_LINESTRING:
    case SF_LINESTRING25D:
    case SF_LINEARRING:
	return GV_LINE;
    case SF_POLYGON:
    case SF_POLYGON25D:
	return GV_BOUNDARY;
    default:
	break;
    }
    
    return -1;
}

/*!
  \brief Check SF type

  E.g. if <em>type</em> is GV_LINE with two or more segments and the
  start node is identical with the end node, and <em>sftype</em> is
  SF_LINEARRING, functions returns 1, otherwise 0.

  \param Points pointer to line_pnts structure
  \param type feature type (GV_POINT, GV_LINE, ...)
  \param sftype SF type to be checked (SF_POINT, SF_LINE, ...)
  \param with_z non-zero value for 3D data
  
  \return 1 if type is sftype
  \return 0 type differs from sftype
*/
int Vect_sfa_check_line_type(const struct line_pnts *Points, int type,
			     SF_FeatureType sftype, int with_z)
{
    return check_sftype(Points, type, sftype, with_z);
}

/*!
  \brief Get geometry dimension

  \param Points pointer to line_pnts structure
  \param type   feature type (GV_POINT, GV_LINE, ...)

  \return 0 for GV_POINT
  \return 1 for GV_LINE
  \return 2 for GV_BOUNDARY
  \return -1 unsupported feature type
*/
int Vect_sfa_line_dimension(int type)
{
    if (type == GV_POINT)
	return 0;
    if (type == GV_LINE)
	return 1;
    if (type == GV_BOUNDARY)
	return 2;

    return -1;
}

/*!
  \brief Get geometry type (string)

  Supported types:
  - GV_POINT             -> SF_POINT      -> "POINT"
  - GV_LINE              -> SF_LINESTRING -> "LINESTRING"
  - GV_LINE (closed)     -> SF_LINEARRING -> "LINEARRING"
  - GV_BOUNDARY (closed) -> SF_POLYGON -> "POLYGON"
  
  Note: Allocated string should be freed by G_free().
  
  \param Points pointer to line_pnts structure (feature geometry)
  \param type   feature type (see supported types above)

  \return geometry type string
  \return NULL unsupported feature type
*/
char *Vect_sfa_line_geometry_type(const struct line_pnts *Points, int type)
{
    SF_FeatureType sftype = Vect_sfa_get_line_type(Points, type, 0);

    if (sftype == SF_POINT)
	return G_store("POINT");
    if (sftype == SF_LINESTRING)
	return G_store("LINESTRING");
    if (sftype == SF_LINEARRING)
	return G_store("LINEARRING");
    if (sftype == SF_POLYGON)
        return G_store("POLYGON");
    
    return NULL;
}

/*!
  \brief Export geometry to Well-Known Text
  
  \param Points    pointer to line_pnts structure
  \param type      feature type
  \param with_z    non-zero value for 3D data
  \param precision floating number precision 
  \param[out] file file where to write the output

  \return 0 on success
  \return -1 unsupported feature type
*/
int Vect_sfa_line_astext(const struct line_pnts *Points, int type, int with_z, int precision, FILE *file)
{
    int i, sftype;
    
    sftype = Vect_sfa_get_line_type(Points, type, with_z);
    
    switch(sftype) {
    case SF_POINT: { /* point */
	fprintf(file, "POINT(");
	print_point(Points, 0, with_z, precision, file);
	fprintf(file, ")\n");
	break;
    }
    case SF_LINESTRING: case SF_LINEARRING: /* line */ {
	if (sftype == SF_LINESTRING)
	    fprintf(file, "LINESTRING(");
	else
	    fprintf(file, "LINEARRING(");
	for (i = 0; i < Points->n_points; i++) {
	    print_point(Points, i, with_z, precision, file);
	    if (i < Points->n_points - 1)
		fprintf(file, ", ");
	}
	fprintf(file, ")\n");
	break;
    }
    case SF_POLYGON: /* polygon */ {
	/* write only outter/inner ring */
	fprintf(file, "(");
	for (i = 0; i < Points->n_points; i++) {
	    print_point(Points, i, with_z, precision, file);
	    if (i < Points->n_points - 1)
		fprintf(file, ", ");
	}
	fprintf(file, ")");
	break;
    }
    default: {
	G_warning(_("Unknown Simple Features type (%d)"), sftype);
	return -1;
    }
    }
    
    fflush(file);
    return 0;
}

/*!
  \brief Check if feature is simple

  \param Points pointer to line_pnts structure
  \param type   feature type (GV_POINT, GV_LINE, ...)
  
  \return 1  feature simple
  \return 0  feature not simple
  \return -1 feature type not supported (GV_POINT, GV_CENTROID, ...)
*/
int Vect_sfa_is_line_simple(const struct line_pnts *Points, int type, int with_z)
{
    SF_FeatureType sftype;
    
    sftype = Vect_sfa_get_line_type(Points, type, with_z);

    /* TODO */

    return 0;
}

/*!
  \brief Check if feature is closed

  \param Points pointer to line_pnts structure
  \param type   feature type (GV_LINE or GV_BOUNDARY)

  \return 1  feature closed
  \return 0  feature not closed
  \return -1 feature type not supported (GV_POINT, GV_CENTROID, ...)
*/
int Vect_sfa_is_line_closed(const struct line_pnts *Points, int type, int with_z)
{
    int npoints;
    if (type & (GV_LINES)) {
	npoints = Vect_get_num_line_points(Points);
	if (npoints > 2 &&
	    Points->x[0] == Points->x[npoints-1] &&
	    Points->y[0] == Points->y[npoints-1]) {
	    if (!with_z)
		return 1;
	    if (Points->z[0] == Points->z[npoints-1])
		return 1;
	}
	return 0;
    }
    return -1;
}

/*!
  \brief Get number of simple features

  For native format or PostGIS Topology returns -1

  \param Map vector map

  \return number of features
  \return -1 on error
*/
int Vect_sfa_get_num_features(const struct Map_info *Map)
{
    int nfeat;

    nfeat = 0;
    if (Map->format == GV_FORMAT_OGR || Map->format == GV_FORMAT_OGR_DIRECT) {
        /* OGR */
#ifdef HAVE_OGR
        const struct Format_info_ogr *ogr_info;
        
        ogr_info = &(Map->fInfo.ogr);
        
        if (!ogr_info->layer)
            return -1;

        return OGR_L_GetFeatureCount(ogr_info->layer, TRUE);
#else
        G_fatal_error(_("GRASS is not compiled with OGR support"));
        return -1;
#endif
    }
    else if (Map->format == GV_FORMAT_POSTGIS && !Map->fInfo.pg.toposchema_name) {
#ifdef HAVE_POSTGRES
        /* PostGIS */
        char stmt[DB_SQL_MAX];
        
        const struct Format_info_pg *pg_info;

        pg_info = &(Map->fInfo.pg);
        
        if (!pg_info->conn || !pg_info->table_name) {
            G_warning(_("No connection defined"));
            return -1;
        }

        sprintf(stmt, "SELECT count(*) FROM \"%s\".%s", pg_info->schema_name,
                pg_info->table_name);
        nfeat = Vect__execute_get_value_pg(pg_info->conn, stmt);
        if (nfeat < 0) {
            G_warning(_("Unable to get number of simple features"));
            return -1;
        }
#else
        G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
        return -1;
#endif
    }
    else {
        G_warning(_("Unable to report simple features for vector map <%s>"),
                  Vect_get_full_name(Map));
        return -1;
    }

    return nfeat;
}

int check_sftype(const struct line_pnts *points, int type,
		 SF_FeatureType sftype, int with_z)
{
    if (type == GV_POINT && sftype == SF_POINT) {
	return 1;
    }

    if (type == GV_LINE) {
	if (sftype == SF_LINESTRING)
	    return 1;
	
	if (sftype == SF_LINEARRING &&
            Vect_sfa_is_line_closed(points, type, with_z))
            return 1;
    }

    if (type == GV_BOUNDARY) {
	if (sftype == SF_POLYGON &&
	    Vect_sfa_is_line_closed(points, type, 0)) /* force 2D */
	    return 1;
    }

    return 0;
}

int get_sftype(const struct line_pnts *points, int type, int with_z)
{
    if (check_sftype(points, type, SF_POINT, with_z))
	return SF_POINT;

    if (check_sftype(points, type, SF_LINEARRING, with_z))
	return SF_LINEARRING;

    if (check_sftype(points, type, SF_LINESTRING, with_z))
	return SF_LINESTRING;

    if (check_sftype(points, type, SF_POLYGON, with_z))
	return SF_POLYGON;

    return -1;
}

void print_point(const struct line_pnts *Points, int index, int with_z, int precision, FILE *file)
{
    fprintf(file, "%.*f %.*f", precision, Points->x[index], precision, Points->y[index]);
    if (with_z)
	fprintf(file, " %.*f", precision, Points->z[index]);
}
