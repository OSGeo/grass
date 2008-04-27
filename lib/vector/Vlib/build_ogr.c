/*!
  \file build_ogr.c
  
  \brief Vector library - Building topology for OGR
  
  Higher level functions for reading/writing/manipulating vectors.

  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Radim Blazek, Piero Cavalieri
  
  \date 2001-2008
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

/* 
 *   Line offset is
 *      - centroids   : FID
 *      - other types : index of the first record (which is FID) in offset array.
 *
 *   Category: FID, not all layer have FID, OGRNullFID is defined (5/2004) as -1, so FID should be only >= 0
 *
 */

#ifdef HAVE_OGR
#include <ogr_api.h>

extern FILE *Msgout;
extern int prnmsg (char *msg, ...);

/* 
*  This structure keeps info about geometry parts above current geometry, path to curent geometry in the 
*  feature. First 'part' number however is feature Id 
*/
typedef struct {
    int *part;
    int a_parts;
    int n_parts;
} GEOM_PARTS;

/* Init parts */
static void init_parts ( GEOM_PARTS *parts ) 
{
    parts->part = NULL;
    parts->a_parts = parts->n_parts = 0;
}

/* Reset parts */
static void reset_parts ( GEOM_PARTS *parts ) 
{
    parts->n_parts = 0;
}

/* Free parts */
static void free_parts ( GEOM_PARTS *parts ) 
{
    free ( parts->part );
    parts->a_parts = parts->n_parts = 0;
}

/* Add new part number to parts */
static void add_part ( GEOM_PARTS *parts, int part ) 
{
    if ( parts->a_parts == parts->n_parts ) {
	parts->a_parts += 10;
	parts->part = (int *) G_realloc ( (void*) parts->part, parts->a_parts * sizeof(int) );
    }
    parts->part[parts->n_parts] = part;
    parts->n_parts++;
}

/* Remove last part */
static void del_part ( GEOM_PARTS *parts ) 
{
    parts->n_parts--;
}

/* Add parts to offset */
static void add_parts_to_offset ( struct Map_info *Map, GEOM_PARTS *parts) 
{
    int i, j;

    if ( Map->fInfo.ogr.offset_num + parts->n_parts >= Map->fInfo.ogr.offset_alloc ) {
	Map->fInfo.ogr.offset_alloc += parts->n_parts + 1000;
	Map->fInfo.ogr.offset = (int *) G_realloc ( Map->fInfo.ogr.offset,
	                                            Map->fInfo.ogr.offset_alloc * sizeof(int) );
    }
    j = Map->fInfo.ogr.offset_num;
    for ( i = 0; i < parts->n_parts; i++ ) {
        G_debug (4, "add offset %d", parts->part[i] );
        Map->fInfo.ogr.offset[j] = parts->part[i];
	j++;
    }
    Map->fInfo.ogr.offset_num += parts->n_parts;
}

/* add line to support structures */
static int add_line ( struct Map_info *Map, int type, struct line_pnts *Points, 
	              int FID, GEOM_PARTS *parts )
{
    int     line;
    struct  Plus_head *plus;
    long    offset;
    BOUND_BOX box;

    plus = &(Map->plus);

    if ( type != GV_CENTROID ) {
	offset = Map->fInfo.ogr.offset_num; /* beginning in the offset array */
    } else { 
	/* TODO : could be used to statore category ? */
        offset = FID; /* because centroids are read from topology, not from layer */
    }
    G_debug (4, "Register line: FID = %d offset = %ld", FID, offset );
    line = dig_add_line (plus, type, Points, offset );
    G_debug (4, "Line registered with line = %d", line);

    /* Set box */
    dig_line_box (Points, &box);
    if (line == 1) Vect_box_copy (&(plus->box), &box);
    else Vect_box_extend (&(plus->box), &box);

    if ( type != GV_BOUNDARY ) { 
	dig_cidx_add_cat (plus, 1, (int)FID, line, type);
    } else {
	dig_cidx_add_cat (plus, 0, 0, line, type);
    }

    if ( type != GV_CENTROID ) /* because centroids are read from topology, not from layer */ 
        add_parts_to_offset ( Map, parts);

    return line;
}

/* Recursively add geometry to topology */
static int add_geometry ( struct Map_info *Map, OGRGeometryH hGeom, int FID, GEOM_PARTS *parts )
{
    struct  Plus_head *plus;
    int     i, ret;
    int     line;
    int     area, isle, outer_area=0;
    int     lines[1];
    static struct line_pnts **Points = NULL;
    static int alloc_points = 0;
    BOUND_BOX box;
    P_LINE  *Line;
    double   area_size, x, y;
    int      eType, nRings, iPart, nParts, nPoints;
    OGRGeometryH hGeom2, hRing;

    G_debug (4, "add_geometry() FID = %d", FID );
    plus = &(Map->plus);

    if ( !Points ) {
        alloc_points = 1;
	Points = (struct line_pnts **) G_malloc ( sizeof (struct line_pnts *) );
        Points[0] = Vect_new_line_struct ();
    }
    Vect_reset_line ( Points[0] );

    eType = wkbFlatten (OGR_G_GetGeometryType (hGeom));
    G_debug (4, "OGR type = %d", eType);

    switch ( eType ) {
	case wkbPoint:
	    G_debug (4, "Point" );
	    Vect_append_point ( Points[0], OGR_G_GetX(hGeom,0), OGR_G_GetY(hGeom,0), OGR_G_GetZ(hGeom,0) );
	    add_line ( Map, GV_POINT, Points[0], FID, parts );
	    break;

	case wkbLineString:
	    G_debug (4, "LineString" );
	    nPoints = OGR_G_GetPointCount(hGeom);
	    for( i = 0; i < nPoints; i++ ) {
		Vect_append_point ( Points[0], 
			            OGR_G_GetX(hGeom,i), OGR_G_GetY(hGeom,i), OGR_G_GetZ(hGeom,i) );
	    }
	    add_line ( Map, GV_LINE, Points[0], FID, parts );
	    break;

	case wkbPolygon:
	    G_debug (4, "Polygon");

	    nRings = OGR_G_GetGeometryCount (hGeom);
	    G_debug (4, "Number of rings: %d", nRings);

	    /* Alloc space for islands */
	    if ( nRings >= alloc_points ) {
		Points = (struct line_pnts **) G_realloc ( (void*)Points, 
			                                   nRings * sizeof (struct line_pnts *));
		for ( i = alloc_points; i < nRings; i++) {
		    Points[i] = Vect_new_line_struct ();
		}
	    }

	    for (iPart = 0; iPart < nRings; iPart++) {
		hRing = OGR_G_GetGeometryRef (hGeom, iPart);
		nPoints = OGR_G_GetPointCount (hRing);
		G_debug (4, "  ring %d : nPoints = %d", iPart, nPoints );
		
		
                Vect_reset_line ( Points[iPart] );
		for ( i = 0; i < nPoints; i++ ) {
		    Vect_append_point ( Points[iPart], 
			    OGR_G_GetX (hRing, i), OGR_G_GetY (hRing, i), OGR_G_GetZ (hRing, i));
		}

		/* register line */
                add_part ( parts, iPart );
		line = add_line ( Map, GV_BOUNDARY, Points[iPart], FID, parts );
		del_part ( parts );

		/* add area (each inner ring is also area) */
		dig_line_box ( Points[iPart], &box);
		dig_find_area_poly (Points[iPart], &area_size);

		if ( area_size > 0 ) /* clockwise */
		    lines[0] = line;	
		else 
	            lines[0] = -line;	
			
		area = dig_add_area (plus, 1, lines);
		dig_area_set_box ( plus, area, &box );

		/* Each area is also isle */
		lines[0] = -lines[0]; /* island is counter clockwise */	
		
		isle = dig_add_isle (plus, 1, lines);
		dig_isle_set_box (plus, isle, &box);
		
		if ( iPart == 0 ) { /* outer ring */
		    outer_area = area;
		} else { /* inner ring */
    		    P_ISLE  *Isle;

		    Isle = plus->Isle[isle];
		    Isle->area = outer_area;

		    dig_area_add_isle ( plus, outer_area, isle );
		}
	    }

	    /* create virtual centroid */
	    ret = Vect_get_point_in_poly_isl ( Points[0], Points+1, nRings-1, &x, &y );
	    if (ret < -1) {
		G_warning (_("Unable to calculate centroid for area %d"), outer_area );
	    } else { 
                P_AREA  *Area;

		G_debug (4, "  Centroid: %f, %f", x, y );
		Vect_reset_line ( Points[0] );
		Vect_append_point ( Points[0], x, y, 0.0);
		line = add_line ( Map, GV_CENTROID, Points[0], FID, parts );
		dig_line_box (Points[0], &box);
		dig_line_set_box (plus, line, &box);

		Line = plus->Line[line];
		Line->left = outer_area;

		/* register centroid to area */
		Area = plus->Area[outer_area];
		Area->centroid = line;
	    }
	    break;

	case wkbMultiPoint:
	case wkbMultiLineString:
	case wkbMultiPolygon:
	case wkbGeometryCollection:
	    nParts = OGR_G_GetGeometryCount( hGeom );
	    G_debug (4, "%d geoms -> next level", nParts );
	    for ( i = 0 ; i < nParts; i++ ) {
                add_part ( parts, i );
	        hGeom2 = OGR_G_GetGeometryRef( hGeom, i );
                add_geometry ( Map, hGeom2, FID, parts );
		del_part ( parts );
	    }
	    break;

	default:
	    G_warning (_("OGR feature type %d not supported"), eType);
	    break;
    }

    return 0;
}

/*!
  \brief Build topology 

  \param Map_info vector map
  \param build build level 
  \param msgout message output (stdout/stderr for example) or NULL

  \return 1 on success
  \return 0 on error
*/
int
Vect_build_ogr (struct Map_info *Map, int build, FILE * msgout)
{
    int          iFeature, count, FID;
    GEOM_PARTS   parts;
    OGRFeatureH  hFeature;
    OGRGeometryH hGeom;

    if ( build != GV_BUILD_ALL )
      G_fatal_error (_("Partial build for OGR is not supported"));

    Msgout = msgout;

    /* TODO move this init to better place (Vect_open_ ?), because in theory build may be reused on level2 */
    Map->fInfo.ogr.offset = NULL;
    Map->fInfo.ogr.offset_num = 0;
    Map->fInfo.ogr.offset_alloc = 0;

    /* test layer capabilities */
    if ( !OGR_L_TestCapability ( Map->fInfo.ogr.layer, OLCRandomRead ) ) {
	G_warning (_("Random read is not supported by OGR for this layer, cannot build support"));
	return 0;
    }
    
    init_parts (&parts);

    /* Note: Do not use OGR_L_GetFeatureCount (it may scan all features)!!! */
    prnmsg (_("Feature: "));

    OGR_L_ResetReading ( Map->fInfo.ogr.layer );
    count = iFeature = 0;
    while ((hFeature = OGR_L_GetNextFeature ( Map->fInfo.ogr.layer )) != NULL) {
	iFeature++;
	count++;
	
	G_debug (4, "---- Feature %d ----", iFeature);

	/* print progress */
	if ( count == 1000 ) {
	    prnmsg ("%7d\b\b\b\b\b\b\b", iFeature);
	    count = 0;
	}

	hGeom = OGR_F_GetGeometryRef (hFeature);
	if (hGeom == NULL) {
	    G_warning (_("Feature %d without geometry ignored"), iFeature);
	    OGR_F_Destroy( hFeature );
	    continue;
	}

	FID = (int) OGR_F_GetFID ( hFeature );
	if ( FID == OGRNullFID ) {
	    G_warning (_("OGR feature without ID ignored"));
	    OGR_F_Destroy( hFeature );
	    continue;
	}
	G_debug (3, "FID =  %d", FID);

        reset_parts (&parts);
        add_part ( &parts, FID );
        add_geometry ( Map, hGeom, FID, &parts );

	OGR_F_Destroy( hFeature );
    }				/* while */
    free_parts (&parts);

    prnmsg ("%4d\n", iFeature);

    Map->plus.built = GV_BUILD_ALL;
    return 1;
}
#endif
