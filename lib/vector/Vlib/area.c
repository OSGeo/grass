/*!
  \file area.c
  
  \brief Vector library - area feature related fns
  
  Higher level functions for reading/writing/manipulating vectors.

  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Original author CERL, probably Dave Gerdes or Mike Higgins.
  Update to GRASS 5.7 Radim Blazek and David D. Gray.
  
  \date 2001-2008
*/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

/*!
  \brief Returns the polygon array of points in BPoints

  \param Map vector map
  \param area area id
  \param[out] BPoints points array

  \return number of points
  \return -1 on error
*/
int 
Vect_get_area_points (
		       struct Map_info *Map,
		       int area,
		       struct line_pnts *BPoints)
{
  int i, line, aline, dir;
  struct Plus_head *Plus;
  P_AREA *Area;
  static int first_time = 1;
  static struct line_pnts *Points;
  
  G_debug ( 3, "Vect_get_area_points(): area = %d", area );	
  BPoints->n_points = 0;

  Plus = &(Map->plus);
  Area = Plus->Area[area];

  if ( Area == NULL ) { /* dead area */
      G_warning (_("Attempt to read points of nonexisting area")); 
      return -1;      /* error , because we should not read dead areas */
  }
  
  if (first_time == 1){
      Points = Vect_new_line_struct ();	
      first_time = 0;
  }

  G_debug ( 3, "  n_lines = %d", Area->n_lines );	
  for (i = 0; i < Area->n_lines; i++)
    {
      line = Area->lines[i];
      aline = abs (line);
      G_debug ( 3, "  append line(%d) = %d", i, line );	

      if (0 > Vect_read_line (Map, Points, NULL, aline)) {
          G_fatal_error ( "Cannot read line %d",  aline );
      }
      
      G_debug ( 3, "  line n_points = %d", Points->n_points );	

      if ( line > 0 )
          dir = GV_FORWARD;
      else 
	  dir = GV_BACKWARD;

      Vect_append_points ( BPoints, Points, dir);  
      if ( i != (Area->n_lines - 1) ) /* all but not last */
	 BPoints->n_points--; 
      G_debug ( 3, "  area n_points = %d", BPoints->n_points );	
    }

  return (BPoints->n_points);
}

/*!
  \brief Returns the polygon array of points in BPoints

  \param Map vector map
  \param isle island id
  \param[out] BPoints points array

  \return number of points or -1 on error
*/
int 
Vect_get_isle_points (
		       struct Map_info *Map,
		       int isle,
		       struct line_pnts *BPoints)
{
  int i, line, aline, dir;
  struct Plus_head *Plus;
  P_ISLE *Isle;
  static int first_time = 1;
  static struct line_pnts *Points;
  
  G_debug ( 3, "Vect_get_isle_points(): isle = %d", isle );	
  BPoints->n_points = 0;

  Plus = &(Map->plus);
  Isle = Plus->Isle[isle];

  if (first_time == 1) {
      Points = Vect_new_line_struct ();	
      first_time = 0;
  }

  G_debug ( 3, "  n_lines = %d", Isle->n_lines );	
  for (i = 0; i < Isle->n_lines; i++)
    {
      line = Isle->lines[i];
      aline = abs (line);
      G_debug ( 3, "  append line(%d) = %d", i, line );	

      if (0 > Vect_read_line (Map, Points, NULL, aline)) {
          G_fatal_error (_("Unable to read line %d"),  aline );
      }
      
      G_debug ( 3, "  line n_points = %d", Points->n_points );	

      if ( line > 0 )
          dir = GV_FORWARD;
      else 
	  dir = GV_BACKWARD;

      Vect_append_points ( BPoints, Points, dir);  
      if ( i != (Isle->n_lines - 1) ) /* all but not last */
	 BPoints->n_points--; 
      G_debug ( 3, "  isle n_points = %d", BPoints->n_points );	
    }

  return (BPoints->n_points);
}

/*!
  \brief Returns centroid number of area

  \param Map vector map
  \param area area id

  \return centroid number of area
  \return 0 if no centroid found
*/
int 
Vect_get_area_centroid (
		       struct Map_info *Map,
		       int area )
{
  struct Plus_head *Plus;
  P_AREA *Area;
  
  G_debug ( 3, "Vect_get_area_centroid(): area = %d", area );	

  Plus = &(Map->plus);
  Area = Plus->Area[area];

  if ( Area == NULL )
      G_fatal_error (_("Attempt to read topo for dead area (%d)"), area);
  
  return ( Area->centroid );
}

/*!
  \brief Creates list of boundaries for area

  \param Map vector map
  \param area area id
  \param List pointer to list of boundaries

  \return number of boundaries
*/
int 
Vect_get_area_boundaries (
		       struct Map_info *Map,
		       int area,
                       struct ilist *List )
{
  int i, line;
  struct Plus_head *Plus;
  P_AREA *Area;
  
  G_debug ( 3, "Vect_get_area_boundaries(): area = %d", area );	

  Vect_reset_list ( List );
  
  Plus = &(Map->plus);
  Area = Plus->Area[area];

  if ( Area == NULL )
      G_fatal_error (_("Attempt to read topo for dead area (%d)"), area);

  for (i = 0; i < Area->n_lines; i++) {
      line = Area->lines[i];
      Vect_list_append ( List, line );
  }
   
  return ( List->n_values );
}

/*!
  \brief Creates list of boundaries for isle

  \param Map vector map
  \param isle island number
  \param List pointer to list where boundaries are stored

  \return number of boundaries
*/
int 
Vect_get_isle_boundaries (
		       struct Map_info *Map,
		       int isle,
                       struct ilist *List )
{
  int i, line;
  struct Plus_head *Plus;
  P_ISLE *Isle;
  
  G_debug ( 3, "Vect_get_isle_boundaries(): isle = %d", isle );	

  Vect_reset_list ( List );
  
  Plus = &(Map->plus);
  Isle = Plus->Isle[isle];

  if ( Isle == NULL ) G_fatal_error ( "Attempt to read topo for dead isle (%d)", isle);

  for (i = 0; i < Isle->n_lines; i++) {
      line = Isle->lines[i];
      Vect_list_append ( List, line );
  }
   
  return ( List->n_values );
}

/*!
  \brief Returns number of isles for area
  
  \param Map vector map
  \param area area id

  \return number of isles for area
  \return 0 if area not found
*/
int 
Vect_get_area_num_isles (
		       struct Map_info *Map,
		       int area )
{
  struct Plus_head *Plus;
  P_AREA *Area;
  
  G_debug ( 3, "Vect_get_area_num_isles(): area = %d", area );	

  Plus = &(Map->plus);
  Area = Plus->Area[area];
  
  if ( Area == NULL )
      G_fatal_error (_("Attempt to read topo for dead area (%d)"), area);
  
  G_debug ( 3, "  n_isles = %d", Area->n_isles );	
  
  return ( Area->n_isles );

}

/*!
  \brief Returns isle for area

  \param Map vector map
  \param area area id
  \param isle isle id
  
  \return isles for area
  \return 0 if no isle found
*/
int 
Vect_get_area_isle (
		       struct Map_info *Map,
		       int area,
                       int isle)
{
  struct Plus_head *Plus;
  P_AREA *Area;
  
  G_debug ( 3, "Vect_get_area_isle(): area = %d isle = %d", area, isle );	

  Plus = &(Map->plus);
  Area = Plus->Area[area];
  
  if ( Area == NULL )
      G_fatal_error (_("Attempt to read topo for dead area (%d)"), area);

  G_debug ( 3, "  -> isle = %d", Area->isles[isle] );	
  
  return ( Area->isles[isle] );
}

/*!
  \brief Returns area for isle

  \param Map vector
  \param isle island number

  \return area id
  \return 0 area not found
*/
int 
Vect_get_isle_area ( struct Map_info *Map, int isle)
{
  struct Plus_head *Plus;
  P_ISLE *Isle;
  
  G_debug ( 3, "Vect_get_isle_area(): isle = %d", isle );	

  Plus = &(Map->plus);
  Isle = Plus->Isle[isle];
  
  if ( Isle == NULL )
      G_fatal_error (_("Attempt to read topo for dead isle (%d)"), isle);

  G_debug ( 3, "  -> area = %d", Isle->area );	
  
  return ( Isle->area );
}


/*!
  \brief Calculate area perimeter

  \param Points list of points defining area boundary

  \return area perimeter
*/
double 
Vect_area_perimeter ( struct line_pnts *Points )
{
  return Vect_line_length ( Points );
}


/*!
  \brief Returns 1 if point is in area

  \param Map vector map
  \param area area id
  \param x,y point coordinates

  \return 1 if point is in area
  \return 0 if not 
*/
int 
Vect_point_in_area (
		       struct Map_info *Map,
		       int area,
		       double x, double y)
{
  int    i, isle;
  struct Plus_head *Plus;
  P_AREA *Area;
  int poly;
  
  Plus = &(Map->plus);
  Area = Plus->Area[area];
  if ( Area == NULL ) return 0;

  poly = Vect_point_in_area_outer_ring ( x, y, Map, area);
  if ( poly == 0) return 0; /* includes area boundary (poly == 2), OK? */

  /* check if in islands */
  for (i = 0; i < Area->n_isles; i++) {
      isle = Area->isles[i];
      poly = Vect_point_in_island ( x, y, Map, isle);
      if (poly >= 1) return 0; /* excludes island boundary (poly == 2), OK? */
  }

  return 1;
}

/*!
  \brief Returns area of area without areas of isles

  \param Map vector map
  \param area area id

  \return area of area without areas of isles
*/
double 
Vect_get_area_area (
		       struct Map_info *Map,
		       int area)
{
  struct Plus_head *Plus;
  P_AREA *Area;
  struct line_pnts * Points;
  double size;
  int i;
  static int first_time = 1;
  
  G_debug ( 3, "Vect_get_area_area(): area = %d", area );	

  if (first_time == 1) {
      G_begin_polygon_area_calculations();
      first_time = 0;
  }

  Points = Vect_new_line_struct();
  Plus = &(Map->plus);
  Area = Plus->Area[area];
 
  Vect_get_area_points(Map, area, Points);
  size = G_area_of_polygon(Points->x, Points->y, Points->n_points);

  /* substructing island areas */
  for(i = 0; i < Area->n_isles; i++) {
      Vect_get_isle_points(Map, Area->isles[i], Points);
      size -= G_area_of_polygon(Points->x, Points->y, Points->n_points);
  }
  
  Vect_destroy_line_struct(Points);
  
  G_debug ( 3, "    area = %f", size );	
  
  return ( size );
}

/*!
  \brief Get area categories

  \param Map vector map
  \param area area id
  \param[out] Cats list of categories

  \return 0 OK centroid found (but may be without categories)
  \return 1 no centroid found
*/
int
Vect_get_area_cats ( struct Map_info *Map, int area, struct line_cats *Cats ) 
{
    int centroid;

    Vect_reset_cats ( Cats );
    
    centroid = Vect_get_area_centroid ( Map, area );
    if( centroid > 0 ) {
	Vect_read_line (Map, NULL, Cats, centroid );
    } else {
	return 1; /* no centroid */
    }
    
     
    return 0;
}

/*!
  \brief Find FIRST category of given field and area

  \param Map vector map
  \param area area id
  \param field layer number

  \return first found category of given field
  \return -1 no centroid or no category found
*/
int
Vect_get_area_cat ( struct Map_info *Map, int area, int field ) 
{
    int i;
    static struct line_cats *Cats = NULL;

    if ( !Cats ) 
	Cats = Vect_new_cats_struct();
    else
	Vect_reset_cats ( Cats );

    if ( Vect_get_area_cats ( Map, area, Cats ) == 1 || Cats->n_cats == 0) {
	return -1;
    }
    
    for ( i = 0; i < Cats->n_cats; i++ ) {
	if ( Cats->field[i] == field ) {
	    return Cats->cat[i];
	}
    }
     
    return -1;
}
