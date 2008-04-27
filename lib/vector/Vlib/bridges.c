/*!
  \file bridges.c
  
  \brief Vector library - clean geometry (bridges)
  
  Higher level functions for reading/writing/manipulating vectors.

  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Radim Blazek
  
  \date 2001-2008
*/

#include <stdlib.h> 
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

static void
remove_bridges ( struct Map_info *Map, int chtype, struct Map_info *Err, FILE *msgout );
    
/*!
  \brief Remove bridges from vector map.

  Remove bridges (type boundary) connecting areas to islands or 2 islands.
  Islands and areas must be already clean, i.e. without dangles.
  Bridge may be formed by more lines.
  Optionaly deleted bridges are written to error map. 
  Input map must be opened on level 2 for update at least on level GV_BUILD_BASE
   
  \param Map input map where bridges are deleted
  \param Err vector map where deleted bridges are written or NULL
  \param msgout file pointer where messages will be written or NULL
  
  \return
*/

void
Vect_remove_bridges ( struct Map_info *Map, struct Map_info *Err, FILE *msgout )
{
    remove_bridges ( Map, 0, Err, msgout );
}

/*!
  \brief Change type of bridges in vector map.

  Change the type of bridges (type boundary) connecting areas to islands or 2 islands.
  Islands and areas must be already clean, i.e. without dangles.
  Bridge may be formed by more lines.
  Optionaly changed bridges are written to error map. 
  Input map must be opened on level 2 for update at least on level GV_BUILD_BASE.
   
  \param Map input map where bridges are changed
  \param Err vector map where changed bridges are written or NULL
  \param msgout file pointer where messages will be written or NULL

  \return
*/

void
Vect_chtype_bridges ( struct Map_info *Map, struct Map_info *Err, FILE *msgout )
{
    remove_bridges ( Map, 1, Err, msgout );
}

/* 
   Called by Vect_remove_bridges() and Vect_chtype_bridges():
   chtype = 0 -> works like Vect_remove_bridges()
   chtype = 1 -> works like Vect_chtype_bridges()

   Algorithm: Go thorough all lines, 
              if both sides of the line have left and side 0 (candidate) do this check:
	      follow adjacent lines in one direction (nearest to the right at the end node),
	      if we reach this line again without dangle in the way, but with this line 
	      traversed from other side it is a bridge.
              
	      List of all lines in chain is created during the cycle.
*/
void
remove_bridges ( struct Map_info *Map, int chtype, struct Map_info *Err, FILE *msgout )
{
    int  i, type, nlines, line;
    int  left, right, node1, node2, current_line, next_line;
    int  bridges_removed = 0; /* number of removed bridges */
    int  lines_removed = 0; /* number of lines removed */
    char *lmsg;
    struct Plus_head *Plus;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct ilist *CycleList;
    struct ilist *BridgeList;

    int dangle, other_side; 

    if ( chtype )
        lmsg = "changed lines";
    else
	lmsg = "removed lines";

    Plus = &(Map->plus);
    
    Points = Vect_new_line_struct ();
    Cats = Vect_new_cats_struct ();
    CycleList = Vect_new_list ();
    BridgeList = Vect_new_list ();

    nlines = Vect_get_num_lines (Map);

    G_debug (1, "nlines =  %d", nlines );

    if ( msgout ) fprintf (msgout, "%s: %5d  %s: %5d",
			   _("Removed bridges"), bridges_removed, lmsg, lines_removed );
    
    for ( line = 1; line <= nlines; line++ ){ 
	if ( !Vect_line_alive ( Map, line ) ) continue;

	type = Vect_read_line (Map, NULL, NULL, line);
	if ( !(type & GV_BOUNDARY ) ) continue;
	
	Vect_get_line_areas ( Map, line, &left, &right );

	if ( left != 0 || right != 0 ) continue; /* Cannot be bridge */

	G_debug (2, "line %d - bridge candidate", line );
	
	Vect_get_line_nodes ( Map, line, &node1, &node2 );

	if ( abs(node1) == abs(node2) ) continue; /* either zero length or loop -> cannot be a bridge */

	current_line = -line; /* we start with negative (go forward, node2 ) */

	dangle = 0;
	other_side = 0;
	Vect_reset_list ( CycleList );
	Vect_reset_list ( BridgeList );
	while ( 1 ) {
	    next_line = dig_angle_next_line ( Plus, current_line, GV_RIGHT, GV_BOUNDARY );

	    /* Add this line to the list */
	    if ( Vect_val_in_list ( CycleList, abs(next_line) ) ) /* other side -> bridge chain */
	        Vect_list_append ( BridgeList, abs(next_line) );
	    else	
	        Vect_list_append ( CycleList, abs(next_line) );
	    
	    if ( abs(next_line) == abs(current_line) ) {
	        G_debug (4, "  dangle -> no bridge" );
		dangle = 1;
		break;
	    }
	    if ( abs(next_line) == line ) { /* start line reached */
		/* which side */
		if ( next_line < 0 ) { /* other side (connected by node 2) */
	            G_debug (5, "  other side reached" );
		    other_side = 1;
		} else { /* start side */
		    break;
		}
	    }
	    
	    current_line = -next_line; /* change the sign to look at the next node in following cycle */
	}

	if ( !dangle && other_side ) {
	    G_debug (3, " line %d is part of bridge chain", line );

	    for ( i = 0; i < BridgeList->n_values; i++) {
		Vect_read_line (Map, Points, Cats, BridgeList->value[i]);
		
		if ( Err ) {
		    Vect_write_line ( Err, GV_BOUNDARY, Points, Cats );
		}
		
		if ( !chtype )
		    Vect_delete_line (Map, BridgeList->value[i]);
                else 
	            Vect_rewrite_line ( Map, BridgeList->value[i], GV_LINE, Points, Cats);

		lines_removed++;
            }
	    bridges_removed++;
	}

	if ( msgout ) {
	    fprintf (msgout, "\r%s: %5d  %s: %5d",
		     _("Removed bridges"), bridges_removed, lmsg, lines_removed );
	    fflush ( msgout );
	}
    }
    if ( msgout ) {
	fprintf (msgout, "\r%s: %5d  %s: %5d",
		 _("Removed bridges"), bridges_removed, lmsg, lines_removed );
	fprintf (msgout, "\n" );
    }
}
