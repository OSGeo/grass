/*!
  \file remove_areas.c
  
  \brief Vector library - clean geometry (remove small areas)
  
  Higher level functions for reading/writing/manipulating vectors.

  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Radim Blazek
  
  \date 2001
*/

#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

/*!
  \brief Remove small areas from the map map.

  Centroid of the area and the longest boundary 
  with adjacent area is removed.
  Map topology must be built GV_BUILD_CENTROIDS.

  \param Map vector map
  \param thresh maximum area size for removed areas
  \param Err vector map where removed lines and centroids are written
  \param msgout file pointer where messages will be written or NULL
  \param removed_area  pointer to where total size of removed area is stored or NULL

  \return number of removed areas 
*/
int 
Vect_remove_small_areas ( struct Map_info *Map, double thresh, struct Map_info *Err, FILE *msgout, 
                          double *removed_area )
{
    int area;
    int nremoved = 0;
    struct ilist *List;
    struct ilist *AList;
    struct line_pnts *Points;
    struct line_cats *Cats;
    double size_removed = 0.0;

    List = Vect_new_list ();
    AList = Vect_new_list ();
    Points = Vect_new_line_struct ();
    Cats = Vect_new_cats_struct ();

    if ( msgout ) fprintf (msgout, "%s: %5d", _("Removed areas"), nremoved ); 
    
    for ( area = 1; area <= Vect_get_num_areas(Map); area++ ){ 
	int i, j, centroid, dissolve_neighbour;
	double length, size;
	
	G_debug (3, "area = %d", area );
	if ( !Vect_area_alive ( Map, area ) ) continue;

	size = Vect_get_area_area(Map,area);
	if ( size > thresh ) continue;
	size_removed += size;
	
	/* The area is smaller than the limit -> remove */

	/* Remove centroid */
	centroid = Vect_get_area_centroid (  Map, area );
	if ( centroid > 0 ) {
	    if ( Err ) {
		Vect_read_line ( Map, Points, Cats, centroid );
		Vect_write_line ( Err, GV_CENTROID, Points, Cats );
	    }
	    Vect_delete_line (Map, centroid); 
	}

	/* Find the adjacent area with which the longest boundary is shared */
        
	Vect_get_area_boundaries ( Map, area, List );

	/* Create a list of neighbour areas */
	Vect_reset_list ( AList );
	for ( i = 0; i < List->n_values; i++ ) {
	    int line, left, right, neighbour;

	    line = List->value[i];
	    
	    if ( !Vect_line_alive(Map,abs(line)) ) /* Should not happen */
		G_fatal_error (_("Area is composed of dead boundary"));

	    Vect_get_line_areas ( Map, abs(line), &left, &right );
	    if ( line > 0 ) neighbour = left;
	    else neighbour = right;
	    
	    G_debug (4, "  line = %d left = %d right = %d neighbour = %d", line, left, right, neighbour );
		
	    Vect_list_append ( AList, neighbour ); /* this checks for duplicity */
	}
	G_debug (3, "num neighbours = %d", AList->n_values );

	/* Go through the list of neghours and find that with the longest boundary */
	dissolve_neighbour = 0;
	length = -1.0;
	for ( i = 0; i < AList->n_values; i++ ) {
	    int neighbour1;
	    double l = 0.0;

	    neighbour1 = AList->value[i];
	    G_debug (4, "   neighbour1 = %d", neighbour1 );
		
	    for ( j = 0; j < List->n_values; j++ ) {
		int line, left, right, neighbour2;

	        line = List->value[j];
	        Vect_get_line_areas ( Map, abs(line), &left, &right );
		if ( line > 0 ) neighbour2= left;
		else neighbour2 = right;

		if ( neighbour2 == neighbour1 ) {
		    Vect_read_line ( Map, Points, NULL, abs(line) );
		    l += Vect_line_length ( Points );
		}
	    }
	    if ( l > length ) {
		length = l;
		dissolve_neighbour = neighbour1;
	    }
	}

	G_debug (3, "dissolve_neighbour = %d", dissolve_neighbour );

	/* Make list of boundaries to be removed */
	Vect_reset_list ( AList );
	for ( i = 0; i < List->n_values; i++ ) {
	    int line, left, right, neighbour;

	    line = List->value[i];
	    Vect_get_line_areas ( Map, abs(line), &left, &right );
	    if ( line > 0 ) neighbour= left;
	    else neighbour = right;
	    
	    G_debug (3, "   neighbour = %d", neighbour );

	    if ( neighbour == dissolve_neighbour ) {
		Vect_list_append ( AList, abs(line) );
	    }
	}
	
	/* Remove boundaries */
	for ( i = 0; i < AList->n_values; i++ ) {
	    int line;

	    line = AList->value[i];

	    if ( Err ) {
		Vect_read_line ( Map, Points, Cats, line );
		Vect_write_line ( Err, GV_BOUNDARY, Points, Cats );
	    }
	    Vect_delete_line ( Map, line );
	}

	nremoved++;
	if ( msgout ) {
	    fprintf (msgout, "\r%s: %5d", _("Removed areas"), nremoved ); 
	    fflush ( stderr );
	}
    }
    if ( msgout ) fprintf (stderr, "\n" ); 

    if ( removed_area )
	*removed_area = size_removed;

    return (nremoved);
}
