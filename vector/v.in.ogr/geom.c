/****************************************************************
 *
 * MODULE:       v.in.ogr
 * 
 * AUTHOR(S):    Radim Blazek
 *               
 * PURPOSE:      Import OGR vectors
 *               
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/

#include <stdlib.h> 
#include <string.h> 
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "ogr_api.h"
#include "global.h"


/* Add categories to centroids inside polygon */
int 
centroid(OGRGeometryH hGeom, CENTR *Centr, SPATIAL_INDEX *Sindex, int field, int cat, double min_area, int type)
{
    int    i, j, np, nr, ret;
    static int first = 1;
    static struct line_pnts *Points;
    struct line_pnts **IPoints;
    static struct line_cats *BCats, *Cats ;
    OGRwkbGeometryType eType;
    OGRGeometryH hRing;
    double size;
    static struct ilist *List;
    BOUND_BOX box;

    G_debug ( 3, "centroid() cat = %d", cat );
    
    if ( first ) {
	Points = Vect_new_line_struct ();
	BCats = Vect_new_cats_struct ();
	Cats = Vect_new_cats_struct ();
	List = Vect_new_list ();
	first = 0;
    } else {
	Vect_reset_line ( Points );
	Vect_reset_cats ( Cats );
	Vect_reset_cats ( BCats );
	Vect_cat_set ( Cats, field, cat );
    }

    eType = wkbFlatten(OGR_G_GetGeometryType(hGeom));

    if( eType == wkbPolygon ) {
        nr = OGR_G_GetGeometryCount( hGeom );

        G_debug ( 3, "polygon: %d rings", nr );
	
	/* SFS: 1 exterior boundary and 0 or more interior boundaries.
	*  So I hope that exterior is the first one, even if it is not explicitly told  */

	/* Area */
	hRing = OGR_G_GetGeometryRef( hGeom, 0 );
	np = OGR_G_GetPointCount(hRing);
	Vect_reset_line ( Points );
	for( j = 0; j < np; j++ ) {
	    Vect_append_point ( Points, OGR_G_GetX(hRing,j), 
		    OGR_G_GetY(hRing,j), OGR_G_GetZ(hRing,j) );
	}
	
	/* Degenerate is ignored */ 
	if ( Points->n_points < 4 ) return 0;
	
	/* Small areas ignored because boundaries are not imported */
	size = G_area_of_polygon(Points->x, Points->y, Points->n_points);
	if ( size < min_area ) return 0;
	
	/* Isles */
	IPoints = (struct line_pnts **) G_malloc ( (nr-1) * sizeof (struct line_pnts *) );
        for( i = 1; i < nr; i++ ) {
	    
	    IPoints[i-1] = Vect_new_line_struct ();
            hRing = OGR_G_GetGeometryRef( hGeom, i );
            np = OGR_G_GetPointCount(hRing);

            for( j = 0; j < np; j++ ) {
                Vect_append_point ( IPoints[i-1], OGR_G_GetX(hRing,j), 
			OGR_G_GetY(hRing,j), OGR_G_GetZ(hRing,j) );
	    }
        }

	/* Find centroids */
	if ( Points->n_points >= 4 ) {
	    int centr, in;
	    double x,y;
	    
	    Vect_line_box (Points, &box);
	    Vect_spatial_index_select (Sindex, &box, List);
	    
	    for( i = 0; i < List->n_values; i++ ) { 
		centr = List->value[i];
		x = Centr[centr].x;
		y = Centr[centr].y;
		ret = Vect_point_in_poly ( x, y, Points);
		if ( ret == 0 ) continue; /* outside */
		
		in = 1;
	        for( j = 1; j < nr; j++ ) {
		    ret = Vect_point_in_poly ( x, y, IPoints[j-1] );
		    if ( ret == 1 ) { /* centroid in inner ring */
			in = 0;
			break; /* inside isle */
		    }
		}
		if ( !in ) continue;

		G_debug ( 3, "Centroid %d : layer %d cat %d", centr, field, cat );
		Vect_cat_set ( Centr[centr].cats, field, cat);
	    }
	} 

	for( i = 1; i < nr; i++ ) {
            Vect_destroy_line_struct ( IPoints[i-1] );
	}
        if ( nr > 1 ) G_free ( IPoints );	 
    }

    /* I did not test this because I did not have files of these types */
    else if( eType == wkbGeometryCollection || eType == wkbMultiPolygon ) {
	G_debug (3, "GeometryCollection or MultiPolygon/LineString/Point" );
        nr = OGR_G_GetGeometryCount( hGeom );
        for( i = 0; i < nr; i++ ) {
            hRing = OGR_G_GetGeometryRef( hGeom, i );

	    ret = centroid( hRing, Centr, Sindex, field, cat, min_area, type );
        }
    }
   
    return 0;
}

/* Write geometry to output map */
int 
geom(OGRGeometryH hGeom, struct Map_info *Map, int field, int cat, double min_area, int type, int mk_centr)
{
    int    i, j, np, nr, ret, otype;
    static int first = 1;
    static struct line_pnts *Points;
    struct line_pnts **IPoints;
    static struct line_cats *BCats, *Cats ;
    OGRwkbGeometryType eType;
    OGRGeometryH hRing;
    double  x, y;
    double size;

    G_debug (3, "geom() cat = %d", cat );
    
    if ( first ) {
	Points = Vect_new_line_struct ();
	BCats = Vect_new_cats_struct ();
	Cats = Vect_new_cats_struct ();
	first = 0;
    }
    Vect_reset_line ( Points );
    Vect_reset_cats ( Cats );
    Vect_reset_cats ( BCats );
    Vect_cat_set ( Cats, field, cat );

    eType = wkbFlatten(OGR_G_GetGeometryType(hGeom));

    if( eType == wkbPoint ) {
         Vect_append_point ( Points, OGR_G_GetX(hGeom,0), OGR_G_GetY(hGeom,0), OGR_G_GetZ(hGeom,0) );
	 if ( type & GV_CENTROID ) otype = GV_CENTROID; else otype = GV_POINT; 
         Vect_write_line ( Map, otype, Points, Cats);
    }
    else if( eType == wkbLineString )
    {
        np = OGR_G_GetPointCount(hGeom);

        for( i = 0; i < np; i++ ) {
            Vect_append_point ( Points, OGR_G_GetX(hGeom,i), 
		    OGR_G_GetY(hGeom,i), OGR_G_GetZ(hGeom,i) );
	}
	 if ( type & GV_BOUNDARY ) otype = GV_BOUNDARY; else otype = GV_LINE; 
        Vect_write_line ( Map, otype, Points, Cats);
    }

    else if( eType == wkbPolygon )
    {
	G_debug (3, "Polygon" );

	n_polygons++;
        nr = OGR_G_GetGeometryCount( hGeom );

	/* SFS: 1 exterior boundary and 0 or more interior boundaries.
	*  So I hope that exterior is the first one, even if it is not explicitly told  */

	/* Area */
	hRing = OGR_G_GetGeometryRef( hGeom, 0 );
	np = OGR_G_GetPointCount(hRing);
	Vect_reset_line ( Points );
	for( j = 0; j < np; j++ ) {
	    Vect_append_point ( Points, OGR_G_GetX(hRing,j), 
		    OGR_G_GetY(hRing,j), OGR_G_GetZ(hRing,j) );
	}
	
	/* Degenerate is not ignored because it may be useful to see where it is,
	 * but may be eliminated by min_area option */
	if ( Points->n_points < 4 ) 
	    G_warning (_("Degenerate polygon ([%d] vertices)"), Points->n_points );
	
	size = G_area_of_polygon(Points->x, Points->y, Points->n_points);
	if ( size < min_area ) {
	    G_warning (_("Area size [%.1e], area not imported"), size);
	    return 0;
	}
	
	if ( type & GV_LINE ) otype = GV_LINE; else otype = GV_BOUNDARY; 
        Vect_write_line ( Map, otype, Points, BCats);
	
	/* Isles */
	IPoints = (struct line_pnts **) G_malloc ( (nr-1) * sizeof (struct line_pnts *) );
        for( i = 1; i < nr; i++ ) {
	    G_debug (3, "Inner ring %d", i );
	    
	    IPoints[i-1] = Vect_new_line_struct ();
            hRing = OGR_G_GetGeometryRef( hGeom, i );
            np = OGR_G_GetPointCount(hRing);

            for( j = 0; j < np; j++ ) {
                Vect_append_point ( IPoints[i-1], OGR_G_GetX(hRing,j), 
			OGR_G_GetY(hRing,j), OGR_G_GetZ(hRing,j) );
	    }
	    
	    if ( IPoints[i-1]->n_points < 4 ) 
	        G_warning (_("Degenerate island ([%d] vertices)"), IPoints[i-1]->n_points );
	    
	    size = G_area_of_polygon(Points->x, Points->y, Points->n_points);
	    if ( size < min_area ) {
		G_warning (_("Island size [%.1e], island not imported"), size);
	    } else { 
	        if ( type & GV_LINE ) otype = GV_LINE; else otype = GV_BOUNDARY; 
                Vect_write_line ( Map, otype, IPoints[i-1], BCats);
	    }
        }

	/* Centroid */
	/* Vect_get_point_in_poly_isl() would fail for degenerate polygon */
	if ( mk_centr ) {
	    if ( Points->n_points >= 4 ) {
		ret = Vect_get_point_in_poly_isl ( Points, IPoints, nr-1, &x, &y);
		if ( ret == -1 ) {
		    G_warning (_("Cannot calculate centroid"));
		} else {
		    Vect_reset_line ( Points );
		    Vect_append_point ( Points, x, y, 0.0 );
		    if ( type & GV_POINT ) otype = GV_POINT; else otype = GV_CENTROID; 
		    Vect_write_line ( Map, otype, Points, Cats);
		}
	    } else if ( Points->n_points > 0 ) {
		if ( Points->n_points >= 2 ) { 
		/* center of 1. segment ( 2. point is not best for 3 vertices as 3. may be the same as 1.) */
		    x = (Points->x[0] + Points->x[1]) / 2;  
		    y = (Points->y[0] + Points->y[1]) / 2;
		} else {  /* one point */
		    x = Points->x[0];  
		    y = Points->y[0];
		}
		Vect_reset_line ( Points );
		Vect_append_point ( Points, x, y, 0.0 );
		if ( type & GV_POINT ) otype = GV_POINT; else otype = GV_CENTROID; 
		Vect_write_line ( Map, otype, Points, Cats);
	    } else { /* 0 points */
		G_warning (_("No centroid written for polygon with 0 vertices"));
	    } 
	}
	
	for( i = 1; i < nr; i++ ) {
            Vect_destroy_line_struct ( IPoints[i-1] );
	}
        G_free ( IPoints );	 
    }

    /* I did not test this because I did not have files of these types */
    else if( eType == wkbGeometryCollection
             || eType == wkbMultiPolygon
             || eType == wkbMultiLineString
             || eType == wkbMultiPoint )
    {
	G_debug (3, "GeometryCollection or MultiPolygon/LineString/Point" );
        nr = OGR_G_GetGeometryCount( hGeom );
        for( i = 0; i < nr; i++ ) {
            hRing = OGR_G_GetGeometryRef( hGeom, i );

	    ret = geom( hRing, Map, field, cat, min_area, type, mk_centr );
	    if ( ret == -1 ) {
		G_warning (_("Cannot write part of geometry"));
	    }
        }
    }
   
    else { 
	G_fatal_error (_("Unknown geometry type"));
    }

    return 0;
}
