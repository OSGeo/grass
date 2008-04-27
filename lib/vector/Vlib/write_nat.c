/*!
  \file write_nat.c
  
  \brief Vector library - write data
  
  Higher level functions for reading/writing/manipulating vectors.

  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Original author CERL, probably Dave Gerdes or Mike Higgins.
  Update to GRASS 5.7 Radim Blazek and David D. Gray.
  
  \date 2001
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

static void delete_area_cats_from_cidx ( struct Map_info *Map, int area ) 
{
    int i;
    P_AREA *Area;
    static struct line_cats *Cats = NULL;
    
    G_debug ( 3, "delete_area_cats_from_cidx() area = %d", area );

    Area =  Map->plus.Area[area];
    if ( !Area ) G_fatal_error (_("BUG (delete_area_cats_from_cidx): Area %d does not exist"), area);

    if ( Area->centroid == 0 ) return;

    if ( !Cats ) Cats = Vect_new_cats_struct ();

    V2_read_line_nat ( Map, NULL, Cats, Area->centroid );

    for ( i = 0 ; i < Cats->n_cats; i++ ) {
        dig_cidx_del_cat ( &(Map->plus), Cats->field[i], Cats->cat[i], area, GV_AREA );
    }
}

static void add_area_cats_to_cidx ( struct Map_info *Map, int area ) 
{
    int i;
    P_AREA *Area;
    static struct line_cats *Cats = NULL;
    
    G_debug ( 3, "add_area_cats_to_cidx() area = %d", area );

    Area =  Map->plus.Area[area];
    if ( !Area ) G_fatal_error (_("BUG (add_area_cats_to_cidx): Area %d does not exist"), area );

    if ( Area->centroid == 0 ) return;

    if ( !Cats ) Cats = Vect_new_cats_struct ();

    V2_read_line_nat ( Map, NULL, Cats, Area->centroid );

    for ( i = 0 ; i < Cats->n_cats; i++ ) {
        dig_cidx_add_cat_sorted ( &(Map->plus), Cats->field[i], Cats->cat[i], area, GV_AREA );
    }
}

long V1__rewrite_line_nat ( struct Map_info *Map, long   offset, int    type,
			    struct line_pnts *points, struct line_cats *cats);

/**
   \brief Writes line to 'coor' file

   \param Map pointer to vector map
   \param type feature type
   \param points line geometry
   \param cats line cats

   \return offset into file
   \return -1 on error
*/
long 
V1_write_line_nat (  struct Map_info *Map,
		     int    type,
		     struct line_pnts *points,
		     struct line_cats *cats)
{
  long offset;

  if ( dig_fseek ( &(Map->dig_fp), 0L, SEEK_END) == -1 ) /* set to  end of file */
      return -1;
  
  offset = dig_ftell ( &(Map->dig_fp) );
  if ( offset == -1 )
      return -1;
  
  return V1__rewrite_line_nat (Map, offset, type, points, cats);
}

/**
   \brief Writes line to 'coor' file.

   \param Map pointer to vector map
   \param type feature type
   \param points line geometry
   \param cats line cats

   \return  number of new line
   \return -1 on error
*/
long 
V2_write_line_nat (  struct Map_info *Map,
		     int    type,
		     struct line_pnts *points,
		     struct line_cats *cats)
{
    int    i, s, n, line=0, next_line, area=0, node, side, first, sel_area;
    int    new_area[2];
    long   offset;
    struct Plus_head *plus;
    BOUND_BOX box, abox;
    P_LINE *Line, *NLine;  
    P_AREA *Area;  
    P_NODE *Node;  

    G_debug ( 3, "V2_write_line_nat()" );
    offset = V1_write_line_nat ( Map, type, points, cats);
    if ( offset < 0 ) return -1;

    /* Update topology */
    plus = &(Map->plus);
    /* Add line */
    if (  plus->built >= GV_BUILD_BASE ) {
	line = dig_add_line ( plus, type, points, offset );
	G_debug ( 3, "  line added to topo with id = %d", line );
	dig_line_box ( points, &box );
	dig_line_set_box (plus, line, &box);
	if ( line == 1 )
	    Vect_box_copy (&(plus->box), &box);
	else
	    Vect_box_extend (&(plus->box), &box);
    }

    /* Update areas. Areas are modified if: 
    *  1) first or/and last point are existing nodes ->
    *     - drop areas/islands whose boundaries are neighbour to this boundary at these nodes
    *     - try build areas and islands for this boundary and neighbour boundaries going through these nodes
    *       Question: may be by adding line created new area/isle which doesn't go through nodes of this line
    *            old         new line 
    *        +----+----+                    +----+----+                 +----+----+ 
    *        | A1 | A2 |  +      /      ->  | A1 |   /|   or +   \   -> | A1 | A2 |\
    *        |    |    |                    |    |    |                 |    |    |
    *        +----+----+                    +----+----+                 +----+----+
    *          I1   I1                        I1   I1                      
    *        
    *     - reattache all centroids/isles inside new area(s)
    *     - attach new isle to area outside
    *  2) line is closed ring (node at the end is new, so it is not case above)
    *     - build new area/isle
    *     - check if it is island or contains island(s)
    *     - reattache all centroids/isles inside new area(s)
    *     - attach new isle to area outside
    *
    *  Note that 1) and 2) is done by the same code.
    */
    
    if (  plus->built >= GV_BUILD_AREAS ) {
	if ( type == GV_BOUNDARY ) {
	    Line = plus->Line[line]; 
	    /* Delete neighbour areas/isles */
	    first = 1;
	    for ( s = 1; s < 3; s++ ) { /* for each node */
		if ( s == 1 ) node = Line->N1; /* Node 1 */
		else node = Line->N2;
		G_debug ( 3, "  delete neighbour areas/iseles: side = %d node = %d", s, node );
		Node = plus->Node[node];
		n = 0;
		for (i = 0; i < Node->n_lines; i++) {
		    NLine = plus->Line[abs(Node->lines[i])];
		    if ( NLine->type == GV_BOUNDARY ) 
			n++;
		}
		
		G_debug ( 3, "  number of boundaries at node = %d", n );
		if ( n > 2) { /* more than 2 boundaries at node ( >= 2 old + 1 new ) */
		    /* Line above (to the right), it is enough to check to the right, because if area/isle
		    *  exists it is the same to the left */
		    if ( s == 1 )
			next_line = dig_angle_next_line (plus, line, GV_RIGHT, GV_BOUNDARY ); 
		    else
			next_line = dig_angle_next_line (plus, -line, GV_RIGHT, GV_BOUNDARY ); 

		    if ( next_line != 0 ) { /* there is a boundary to the right */
			NLine = plus->Line[abs(next_line)]; 
			if ( next_line > 0 )      /* the boundary is connected by 1. node */
			    area = NLine->right;  /* we are interested just in this side (close to our line) */
			else if ( next_line < 0 ) /* the boundary is connected by 2. node */
			    area = NLine->left;   
			
			G_debug ( 3, "  next_line = %d area = %d", next_line, area);
			if ( area > 0 ) {         /* is area */
			    Vect_get_area_box ( Map, area, &box); 
			    if ( first ) { Vect_box_copy ( &abox, &box); first = 0; } 
			    else  Vect_box_extend ( &abox, &box);

  			    if ( plus->update_cidx ) {
			        delete_area_cats_from_cidx ( Map, area );
			    }
			    dig_del_area ( plus, area );
			} else if ( area < 0 ) {  /* is isle */
			    dig_del_isle ( plus, -area ); 
			}
		    }
		}
	    }
	    /* Build new areas/isles. Thas true that we deleted also adjacent areas/isles, but if
	    *  they form new one our boundary must participate, so we need to build areas/isles
	    *  just for our boundary */
	    for (s = 1; s < 3; s++) {
		if ( s == 1 ) side = GV_LEFT;
		else side = GV_RIGHT;
		G_debug ( 3, "  build area/isle on side = %d", side );
		
		G_debug ( 3, "Build area for line = %d, side = %d", line, side );
		area = Vect_build_line_area ( Map, line, side );
		G_debug ( 3, "Build area for line = %d, side = %d", line, side );
		if ( area > 0 ) { /* area */
		    Vect_get_area_box ( Map, area, &box); 
		    if ( first ) { Vect_box_copy ( &abox, &box); first = 0; } 
		    else  Vect_box_extend ( &abox, &box);
		} else if ( area < 0 ) { 
		    /* isle -> must be attached -> add to abox */   
		    Vect_get_isle_box ( Map, -area, &box); 
		    if ( first ) { Vect_box_copy ( &abox, &box); first = 0; } 
		    else  Vect_box_extend ( &abox, &box);
		}
		new_area[s-1] = area; 
	    }
	    /* Reattach all centroids/isles in deleted areas + new area.
	    *  Because isles are selected by box it covers also possible new isle created above */
	    if ( !first ) { /* i.e. old area/isle was deleted or new one created */
		/* Reattache isles */
		if (  plus->built >= GV_BUILD_ATTACH_ISLES )
		    Vect_attach_isles ( Map, &abox );
		
		/* Reattach centroids */
		if (  plus->built >= GV_BUILD_CENTROIDS )
		    Vect_attach_centroids ( Map, &abox );
	    }
	    /* Add to category index */
	    if ( plus->update_cidx ) {
	        for (s = 1; s < 3; s++) {
		    if ( new_area[s-1] > 0 ) {
		        add_area_cats_to_cidx ( Map, new_area[s-1] );
		    }
		}
	    }
	}
    }
    
    /* Attach centroid */
    if (  plus->built >= GV_BUILD_CENTROIDS ) {
	if ( type == GV_CENTROID ) {
	    sel_area = Vect_find_area ( Map, points->x[0], points->y[0] );
	    G_debug ( 3, "  new centroid %d is in area %d", line, sel_area);
	    if ( sel_area > 0 ) {
		Area = plus->Area[sel_area];
		Line = plus->Line[line];
		if ( Area->centroid == 0 ) { /* first centroid */
		    G_debug ( 3, "  first centroid -> attach to area");
		    Area->centroid = line;
		    Line->left = sel_area;
		    if ( plus->update_cidx ) {
		        add_area_cats_to_cidx ( Map, sel_area );
		    }
		} else {  /* duplicate centroid */
		    G_debug ( 3, "  duplicate centroid -> do not attach to area");
		    Line->left = -sel_area;
		}
	    }
	}
    }
    
    /* Add cetegory index */  
    for ( i = 0 ; i < cats->n_cats; i++ ) {
        dig_cidx_add_cat_sorted ( plus, cats->field[i], cats->cat[i], line, type );
    }

    G_debug ( 3, "updated lines : %d , updated nodes : %d", plus->n_uplines, plus->n_upnodes );
    return line;
}

/**
   \brief Rewrites line at the given offset.
   
   If the number of points or cats differs from
   the original one or the type is changed:
   GV_POINTS -> GV_LINES or GV_LINES -> GV_POINTS,
   the old one is deleted and the
   new is appended to the end of the file.

   Old line is deleted (marked as dead), new line written.

   \param Map pointer to vector map
   \param offset line offset
   \param type feature type
   \param points line geometry
   \param cats line cats

   \return line offset (rewriten line)
   \return -1 on error
*/
long 
V1_rewrite_line_nat (  struct Map_info *Map,
		       long   offset,
		       int    type,
		       struct line_pnts *points,
		       struct line_cats *cats)
{
  int    old_type;
  struct line_pnts *old_points;
  struct line_cats *old_cats; 
  long   new_offset;
 
  /* TODO: enable points and cats == NULL  */
  
  /* First compare numbers of points and cats with tha old one */
  old_points = Vect_new_line_struct ();
  old_cats = Vect_new_cats_struct ();

  old_type = V1_read_line_nat ( Map, old_points, old_cats, offset );
  if ( old_type == -1 ) return (-1); /* error */

  if ( old_type != -2 /* EOF -> write new line */
       && points->n_points == old_points->n_points 
       && cats->n_cats == old_cats->n_cats
       && (   ( (type & GV_POINTS) && (old_type & GV_POINTS) )   
           || ( (type & GV_LINES ) && (old_type & GV_LINES ) ) ) ) {

      /* equal -> overwrite the old */
      return V1__rewrite_line_nat (Map, offset, type, points, cats);
  } else {
      /* differ -> delete the old and append new */
      /* delete old */
      V1_delete_line_nat ( Map, offset);
      
      /* write new */
      if ( dig_fseek ( &(Map->dig_fp), 0L, SEEK_END) == -1 ) 	/*  end of file */
	  return -1;
      
      new_offset = dig_ftell ( &(Map->dig_fp) );
      if ( new_offset == -1 )
	  return -1;

      return V1__rewrite_line_nat (Map, new_offset, type, points, cats);
  }
}

/**
   \brief Rewrites line of given number.

   Old line is deleted (marked as dead), new line written.

   \param Map pointer to vector map
   \param line line id
   \param type feature type
   \param points line geometry
   \param cats line cats

   \return number of new line
   \return -1 on error
*/
int
V2_rewrite_line_nat (  struct Map_info *Map,
		       int    line,
		       int    type,
		       struct line_pnts *points,
		       struct line_cats *cats)
{
    /* TODO: this is just quick shortcut because we have already V2_delete_nat()
    *        and V2_write_nat() this function first deletes old line
    *        and then writes new one. It is not very effective if number of points
    *        and cats was not changed or topology is not changed (nodes not moved,
    *        angles not changed etc.) */

    V2_delete_line_nat ( Map, line );
  
    return ( V2_write_line_nat ( Map, type, points, cats) );
}

/**
   \brief Rewrites line at the given offset.

   \param Map pointer to vector map
   \param offset line offset
   \param type feature type
   \param points line geometry
   \param cats line cats

   \return line offset
   \return -1 on error
*/
long 
V1__rewrite_line_nat (
		       struct Map_info *Map,
		       long   offset,
		       int    type,
		       struct line_pnts *points,
		       struct line_cats *cats)
{
  int  i, n_points;
  char rhead, nc;
  short field;
  GVFILE *dig_fp;
  
  dig_set_cur_port (&(Map->head.port));
  dig_fp = &(Map->dig_fp);
  
  if ( dig_fseek ( dig_fp, offset, 0) == -1 )
      return -1;

  /* first byte:   0 bit: 1 - alive, 0 - dead
  *                1 bit: 1 - categories, 0 - no category
  *              2-3 bit: store type
  *              4-5 bit: reserved for store type expansion
  *              6-7 bit: not used  
  */
  
  rhead = (char) dig_type_to_store ( type );
  rhead <<= 2;
  if (cats->n_cats > 0) {
      rhead |=  0x02;
  }
  rhead |= 0x01; /* written/rewritten is always alive */
  
  if (0 >= dig__fwrite_port_C (&rhead, 1, dig_fp)) {
    return -1;
  }

  if (cats->n_cats > 0) {
      if ( Map->head.Version_Minor == 1 ) { /* coor format 5.1 */
	  if (0 >= dig__fwrite_port_I ( &(cats->n_cats), 1, dig_fp))
	    return -1;
      } else { /* coor format 5.0 */
	  nc = (char) cats->n_cats;
	  if (0 >= dig__fwrite_port_C (&nc, 1, dig_fp))
	    return -1;
      }

      if (cats->n_cats > 0) {
          if ( Map->head.Version_Minor == 1 ) { /* coor format 5.1 */
	      if (0 >= dig__fwrite_port_I (cats->field, cats->n_cats, dig_fp))
		return -1;
          } else { /* coor format 5.0 */
	      for (i = 0; i < cats->n_cats; i++) { 
		  field = (short) cats->field[i];
		  if (0 >= dig__fwrite_port_S (&field, 1, dig_fp))
		    return -1;
	      }
	  }
	  if (0 >= dig__fwrite_port_I (cats->cat, cats->n_cats, dig_fp))
	    return -1;
      }	
  }
  
  if ( type & GV_POINTS ) {
      n_points = 1;	  
  } else {
      n_points = points->n_points;
      if (0 >= dig__fwrite_port_I (&n_points, 1, dig_fp) )
	  return -1;
  }

  if (0 >= dig__fwrite_port_D (points->x, n_points, dig_fp))
    return -1;
  if (0 >= dig__fwrite_port_D (points->y, n_points, dig_fp))
    return -1;

  if ( Map->head.with_z ) {
      if (0 >= dig__fwrite_port_D (points->z, n_points, dig_fp))
          return -1;
  }
  
  if ( 0 != dig_fflush (dig_fp) )
      return -1;

  return offset;
}

/**
   \brief Deletes line at the given offset.

   \param Map pointer to vector map
   \param offset line offset

   \return  0 ok
   \return -1 on error
*/
int 
V1_delete_line_nat (
		       struct Map_info *Map,
		       long   offset )
{
  char rhead;
  GVFILE *dig_fp;
  
  G_debug ( 3, "V1_delete_line_nat(), offset = %ld", offset );
  
  dig_set_cur_port (&(Map->head.port));
  dig_fp = &(Map->dig_fp);
  
  if ( dig_fseek ( dig_fp, offset, 0) == -1 )
      return -1;

  /* read old */
  if (0 >= dig__fread_port_C (&rhead, 1, dig_fp))
      return (-1);
  
  rhead &= 0xFE; 
  
  if ( dig_fseek (dig_fp, offset, 0) == -1 )
      return -1;

  if (0 >= dig__fwrite_port_C (&rhead, 1, dig_fp))
      return -1;

  if ( 0 != dig_fflush (dig_fp) )
      return -1;

  return 0;
}

/**
   \brief Deletes line of given number.

   \param pointer to vector map
   \param line line id

   \return 0 ok
   \return -1 on error
*/
int 
V2_delete_line_nat ( struct Map_info *Map, int  line )
{
  int ret, i, side, type=0, first=0, next_line, area;
  P_LINE *Line = NULL;
  P_AREA *Area;  
  struct Plus_head *plus;
  BOUND_BOX box, abox;
  int adjacent[4], n_adjacent=0;
  static struct line_cats *Cats = NULL;
  
  G_debug ( 3, "V2_delete_line_nat(), line = %d", line );
  
  plus = &(Map->plus);
  
  if (  plus->built >= GV_BUILD_BASE ) {
      Line = Map->plus.Line[line]; 

      if ( Line == NULL ) G_fatal_error (_("Attempt to delete dead line"));
      type = Line->type;
  }

  if ( !Cats ) {
      Cats = Vect_new_cats_struct ();
  }

  /* Update category index */
  if ( plus->update_cidx ) {
      type = V2_read_line_nat ( Map, NULL, Cats, line );

      for ( i = 0 ; i < Cats->n_cats; i++ ) {
          dig_cidx_del_cat ( plus, Cats->field[i], Cats->cat[i], line, type );
      }
  }
  
  /* delete the line from coor */
  ret = V1_delete_line_nat (Map, Line->offset); 
  
  if ( ret == -1 ) { return ret; }

  /* Update topology */
  if (  plus->built >= GV_BUILD_AREAS && type == GV_BOUNDARY ) {
      /* Store adjacent boundaries at nodes (will be used to rebuild area/isle) */
      /* Adjacent are stored: > 0 - we want right side; < 0 - we want left side */
      n_adjacent = 0;
      
      next_line = dig_angle_next_line (plus, line, GV_RIGHT, GV_BOUNDARY ); 
      if ( next_line != 0 && abs(next_line) != line ) { 
	  /* N1, to the right -> we want the right side for > 0  and left for < 0*/
	  adjacent[n_adjacent] =  next_line;
	  n_adjacent++;
      }
      next_line = dig_angle_next_line (plus, line, GV_LEFT, GV_BOUNDARY ); 
      if ( next_line != 0 && abs(next_line) != line ) { 
	  /* N1, to the left -> we want the left side for > 0  and right for < 0*/
	  adjacent[n_adjacent] =  -next_line;
	  n_adjacent++;
      }
      next_line = dig_angle_next_line (plus, -line, GV_RIGHT, GV_BOUNDARY ); 
      if ( next_line != 0 && abs(next_line) != line ) { 
	  /* N2, to the right -> we want the right side for > 0  and left for < 0*/
	  adjacent[n_adjacent] =  next_line;
	  n_adjacent++;
      }
      next_line = dig_angle_next_line (plus, -line, GV_LEFT, GV_BOUNDARY ); 
      if ( next_line != 0 && abs(next_line) != line ) { 
	  /* N2, to the left -> we want the left side for > 0  and right for < 0*/
	  adjacent[n_adjacent] =  -next_line;
	  n_adjacent++;
      }

      /* Delete area(s) and islands this line forms */
      first = 1;
      if ( Line->left > 0 ) { /* delete area */
	  Vect_get_area_box ( Map, Line->left, &box); 
	  if ( first ) { Vect_box_copy ( &abox, &box); first = 0; } 
	  else  Vect_box_extend ( &abox, &box);
	  
  	  if ( plus->update_cidx ) {
	      delete_area_cats_from_cidx ( Map, Line->left );
	  }
	  dig_del_area ( plus, Line->left );
      } else if ( Line->left < 0 )  {  /* delete isle */
	  dig_del_isle ( plus, -Line->left );
      }
      if ( Line->right > 0 ) { /* delete area */
	  Vect_get_area_box ( Map, Line->right, &box); 
	  if ( first ) { Vect_box_copy ( &abox, &box); first = 0; } 
	  else  Vect_box_extend ( &abox, &box);

  	  if ( plus->update_cidx ) {
	      delete_area_cats_from_cidx ( Map, Line->right );
	  }
	  dig_del_area ( plus, Line->right );
      } else if ( Line->right < 0 )  {  /* delete isle */
	  dig_del_isle ( plus, -Line->right );
      }
  }

  /* Delete reference from area */
  if (  plus->built >= GV_BUILD_CENTROIDS && type == GV_CENTROID ) {
      if ( Line->left > 0 ) {
	  G_debug ( 3, "Remove centroid %d from area %d", line, Line->left );
  	  if ( plus->update_cidx ) {
	      delete_area_cats_from_cidx ( Map, Line->left );
	  }
          Area = Map->plus.Area[Line->left];
	  Area->centroid = 0;
      }  
  }
  
  /* delete the line from topo */
  dig_del_line ( plus, line );

  /* Rebuild areas/isles and attach centroids and isles */
  if ( plus->built >= GV_BUILD_AREAS && type == GV_BOUNDARY ) {
      int *new_areas, nnew_areas;

      nnew_areas = 0;
      new_areas = (int *) G_malloc ( 2 * n_adjacent * sizeof(int) );
      /* Rebuild areas/isles */
      for (i = 0; i < n_adjacent; i++) {
	  if ( adjacent[i] > 0 ) side = GV_RIGHT;
	  else side = GV_LEFT;
	
	  G_debug ( 3, "Build area for line = %d, side = %d", adjacent[i], side );
	  
	  area = Vect_build_line_area ( Map, abs(adjacent[i]), side );
	  if ( area > 0 ) { /* area */
	      Vect_get_area_box ( Map, area, &box); 
	      if ( first ) { Vect_box_copy ( &abox, &box); first = 0; } 
	      else  Vect_box_extend ( &abox, &box);
		    
	      new_areas[nnew_areas] = area;
	      nnew_areas++;
	  } else if ( area < 0 ) { 
	      /* isle -> must be attached -> add to abox */   
	      Vect_get_isle_box ( Map, -area, &box); 
	      if ( first ) { Vect_box_copy ( &abox, &box); first = 0; } 
	      else  Vect_box_extend ( &abox, &box);
	  }
      }
      /* Reattach all centroids/isles in deleted areas + new area.
      *  Because isles are selected by box it covers also possible new isle created above */
      if ( !first ) { /* i.e. old area/isle was deleted or new one created */
          /* Reattache isles */
	  if (  plus->built >= GV_BUILD_ATTACH_ISLES )
	      Vect_attach_isles ( Map, &abox );
	    
	  /* Reattach centroids */
	  if (  plus->built >= GV_BUILD_CENTROIDS )
	      Vect_attach_centroids ( Map, &abox );
      }
      
      if ( plus->update_cidx ) {
	  for (i = 0; i < nnew_areas; i++ ) {
	    add_area_cats_to_cidx ( Map, new_areas[i] );
	  }
      }
  }

  G_debug ( 3, "updated lines : %d , updated nodes : %d", plus->n_uplines, plus->n_upnodes );
  return ret;
}
