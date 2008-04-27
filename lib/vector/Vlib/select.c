/*!
  \file select.c
  
  \brief Vector library - select vector features
  
  Higher level functions for reading/writing/manipulating vectors.

  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Radim Blazek
  
  \date 2001
*/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/Vect.h>

/*!
  \brief Select lines by box.

  Select lines whose boxes overlap specified box!!!
  It means that selected line may or may not overlap the box.

  \param Map vector map
  \param Box bounding box
  \param type line type
  \param[out] list output list, must be initialized

  \return number of lines
*/
int 
Vect_select_lines_by_box (struct Map_info *Map, BOUND_BOX *Box, 
	                            int type, struct ilist *list)
{
    int       i, line, nlines;
    struct    Plus_head *plus ;
    P_LINE    *Line;
    static struct ilist *LocList = NULL;
    
    G_debug ( 3, "Vect_select_lines_by_box()" );
    G_debug ( 3, "  Box(N,S,E,W,T,B): %e, %e, %e, %e, %e, %e", Box->N, Box->S,
                           Box->E, Box->W, Box->T, Box->B);
    plus = &(Map->plus);

    if ( !(plus->Spidx_built) ) {
	G_debug ( 3, "Building spatial index." );
	Vect_build_sidx_from_topo ( Map, NULL );
    }
    
    list->n_values = 0; 
    if ( !LocList ) LocList = Vect_new_list();
    
    nlines = dig_select_lines ( plus, Box, LocList );
    G_debug ( 3, "  %d lines selected (all types)", nlines );

    /* Remove lines of not requested types */
    for ( i = 0; i < nlines; i++ ) { 
	line = LocList->value[i];
	if (  plus->Line[line] == NULL ) continue; /* Should not happen */
        Line = plus->Line[line];
	if ( !(Line->type & type) ) continue; 
	dig_list_add ( list, line ); 
    }

    G_debug ( 3, "  %d lines of requested type", list->n_values );
    
    return list->n_values;
}

/*!
  \brief Select areas by box.

  Select areas whose boxes overlap specified box!!!
  It means that selected area may or may not overlap the box.

  \param Map vector map
  \param Box bounding box
  \param[out] output list, must be initialized

  \return number of areas
*/
int 
Vect_select_areas_by_box (struct Map_info *Map, BOUND_BOX *Box, struct ilist *list)
{
    int i;
    
    G_debug ( 3, "Vect_select_areas_by_box()" );
    G_debug ( 3, "Box(N,S,E,W,T,B): %e, %e, %e, %e, %e, %e", Box->N, Box->S,
                           Box->E, Box->W, Box->T, Box->B);

    if ( !(Map->plus.Spidx_built) ) {
	G_debug ( 3, "Building spatial index." );
	Vect_build_sidx_from_topo ( Map, NULL );
    }

    dig_select_areas ( &(Map->plus), Box, list );
    G_debug ( 3, "  %d areas selected", list->n_values );
    for ( i = 0; i < list->n_values; i++ ) {
        G_debug ( 3, "  area = %d pointer to area structure = %lx", list->value[i], 
		      (unsigned long) Map->plus.Area[list->value[i]] );
            
    }
    return list->n_values;
}


/*!
  \brief Select isles by box.

  Select isles whose boxes overlap specified box!!!
  It means that selected isle may or may not overlap the box.

  \param Map vector map
  \param Box bounding box
  \param[out] list output list, must be initialized

  \return number of isles
*/
int 
Vect_select_isles_by_box (struct Map_info *Map, BOUND_BOX *Box, struct ilist *list)
{
    G_debug ( 3, "Vect_select_isles_by_box()" );
    G_debug ( 3, "Box(N,S,E,W,T,B): %e, %e, %e, %e, %e, %e", Box->N, Box->S,
                           Box->E, Box->W, Box->T, Box->B);

    if ( !(Map->plus.Spidx_built) ) {
	G_debug ( 3, "Building spatial index." );
	Vect_build_sidx_from_topo ( Map, NULL );
    }
    
    dig_select_isles ( &(Map->plus), Box, list );
    G_debug ( 3, "  %d isles selected", list->n_values );
    
    return list->n_values;
}

/*!
  \brief Select nodes by box.

  \param Map vector map
  \param Box bounding box
  \param[out] list output list, must be initialized

  \return number of nodes
*/
int 
Vect_select_nodes_by_box (struct Map_info *Map, BOUND_BOX *Box, struct ilist *list)
{
    struct    Plus_head *plus ;
    
    G_debug ( 3, "Vect_select_nodes_by_box()" );
    G_debug ( 3, "Box(N,S,E,W,T,B): %e, %e, %e, %e, %e, %e", Box->N, Box->S,
                           Box->E, Box->W, Box->T, Box->B);

    plus = &(Map->plus);

    if ( !(plus->Spidx_built) ) {
	G_debug ( 3, "Building spatial index." );
	Vect_build_sidx_from_topo ( Map, NULL );
    }

    list->n_values = 0; 
    
    dig_select_nodes ( plus, Box, list );
    G_debug ( 3, "  %d nodes selected", list->n_values );
    
    return list->n_values;
}

/*!
  \brief Select lines by Polygon with optional isles. 
  
  Polygons should be closed, i.e. first and last points must be identical.

  \param Map vector map
  \param Polygon outer ring
  \param nisles number of islands or 0
  \param Isles array of islands or NULL
  \param type line type
  \param[out] list output list, must be initialised

  \return number of lines
*/
int 
Vect_select_lines_by_polygon ( struct Map_info *Map, struct line_pnts *Polygon, int nisles,
                               struct line_pnts **Isles, int type, struct ilist *List)
{
    int       i;
    BOUND_BOX box;
    static struct line_pnts *LPoints = NULL;
    static struct ilist *LocList = NULL;
    
    /* TODO: this function was not tested with isles */
    G_debug ( 3, "Vect_select_lines_by_polygon() nisles = %d", nisles );

    List->n_values = 0; 
    if ( !LPoints ) LPoints = Vect_new_line_struct();
    if ( !LocList ) LocList = Vect_new_list();
    
    /* Select first all lines by box */
    dig_line_box ( Polygon, &box );
    Vect_select_lines_by_box ( Map, &box, type, LocList);
    G_debug ( 3, "  %d lines selected by box", LocList->n_values );

    /* Check all lines if intersect the polygon */
    for ( i = 0; i < LocList->n_values; i++ ) { 
	int j, line, intersect = 0;
	
	line = LocList->value[i];
	/* Read line points */
	Vect_read_line (Map, LPoints, NULL, line);

	/* Check if any of line vertices is within polygon */
	for (j = 0; j < LPoints->n_points; j++ ) {
	    if ( Vect_point_in_poly(LPoints->x[j], LPoints->y[j], Polygon) >= 1 ) { /* inside polygon */
		int k, inisle = 0;
		
		for (k = 0; k < nisles; k++ ) {
		    if ( Vect_point_in_poly(LPoints->x[j], LPoints->y[j], Isles[k]) >= 1 ) { /* in isle */
		        inisle = 1;
			break;
		    }
		}

		if ( !inisle ) { /* inside polygon, outside isles -> select */
		    intersect = 1;
		    break;
		}
	    }
	}
	if ( intersect ) {
	    dig_list_add ( List, line );
	    continue;
	}

	/* Check intersections of the line with area/isles boundary */
	/* Outer boundary */
	if ( Vect_line_check_intersection(LPoints, Polygon, 0) ) {
	    dig_list_add ( List, line );
	    continue;
	}

	/* Islands */
	for ( j = 0; j < nisles; j++ ) {
	    if ( Vect_line_check_intersection(LPoints, Isles[j], 0) ) {
		intersect = 1;
		break;
	    }
	}
        if ( intersect ) {
            dig_list_add ( List, line );
	}
    }
        
    G_debug ( 4, "  %d lines selected by polygon", List->n_values );
    
    return List->n_values;
}


/*!
  \brief Select areas by Polygon with optional isles. 
  
  Polygons should be closed, i.e. first and last points must be identical.
  
  Warning : values in list may be duplicate!


  \param Map vector map
  \param Polygon outer ring
  \param nisles number of islands or 0
  \param Isles array of islands or NULL
  \param[out] list output list, must be initialised

  \return number of areas
*/
int 
Vect_select_areas_by_polygon ( struct Map_info *Map, struct line_pnts *Polygon, int nisles,
                               struct line_pnts **Isles, struct ilist *List)
{
    int  i, area;
    static struct ilist *BoundList = NULL;
    
    /* TODO: this function was not tested with isles */
    G_debug ( 3, "Vect_select_areas_by_polygon() nisles = %d", nisles );

    List->n_values = 0; 
    if ( !BoundList ) BoundList = Vect_new_list();
    
    /* Select boundaries by polygon */
    Vect_select_lines_by_polygon ( Map, Polygon, nisles, Isles, GV_BOUNDARY, BoundList);

    /* Add areas on left/right side of selected boundaries */
    for ( i = 0; i < BoundList->n_values; i++ ) { 
	int line, left, right;

	line = BoundList->value[i];

	Vect_get_line_areas ( Map, line, &left, &right );
	G_debug (4, "boundary = %d left = %d right = %d", line, left, right );

	if ( left > 0 ) {
            dig_list_add ( List, left );
	} else if ( left < 0 ) { /* island */
	    area = Vect_get_isle_area ( Map, abs(left) );
	    G_debug (4, "  left island -> area = %d", area );
	    if ( area > 0 ) dig_list_add ( List, area );
	}

	if ( right > 0 ) {
            dig_list_add ( List, right );
	} else if ( right < 0 ) { /* island */
	    area = Vect_get_isle_area ( Map, abs(right) );
	    G_debug (4, "  right island -> area = %d", area );
	    if ( area > 0 ) dig_list_add ( List, area );
	}
    }

    /* But the Polygon may be completely inside the area (only one), in that case 
     * we find the area by one polygon point and add it to the list */
    area = Vect_find_area ( Map, Polygon->x[0], Polygon->y[0]);
    if ( area > 0 ) dig_list_add ( List, area );

    G_debug ( 3, "  %d areas selected by polygon", List->n_values );
    
    return List->n_values;
}
