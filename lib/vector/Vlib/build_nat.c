/*!
  \file build_nat.c
  
  \brief Vector library - Building topology for native format
  
  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Original author CERL, probably Dave Gerdes or Mike Higgins.
  Update to GRASS 5.7 Radim Blazek and David D. Gray.
  
  \date 2001-2008
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/Vect.h>

extern FILE *Msgout;
extern int prnmsg ( char *msg, ...) ;

/*!
  \brief Build area on given side of line (GV_LEFT or GV_RIGHT)

  \param Map_info vector map
  \param iline line id
  \param side side (GV_LEFT or GV_RIGHT)

  \return > 0 number of  area
  \return < 0 number of isle
  \return 0 not created (may also already exist)
*/
int
Vect_build_line_area ( struct Map_info *Map, int iline, int side )
{
    int    j, area, isle, n_lines, line, type, direction;
    static int first = 1;
    long   offset;
    struct Plus_head *plus ;
    P_LINE *BLine;
    static struct line_pnts *Points, *APoints;
    plus_t *lines;
    double area_size;

    plus = &(Map->plus);

    G_debug ( 3, "Vect_build_line_area() line = %d, side = %d", iline, side );
    
    if ( first ) {
	Points = Vect_new_line_struct ();
	APoints = Vect_new_line_struct ();
	first = 0;
    }

    area = dig_line_get_area (plus, iline, side); 
    if ( area != 0 ) {
	G_debug ( 3, "  area/isle = %d -> skip", area );
	return 0;
    }
    
    n_lines = dig_build_area_with_line (plus, iline, side, &lines);
    G_debug ( 3, "  n_lines = %d", n_lines );
    if ( n_lines < 1 ) { return 0; } /* area was not built */

    /* Area or island ? */
    Vect_reset_line ( APoints ); 
    for (j = 0; j < n_lines; j++){
	line = abs(lines[j]);
	BLine = plus->Line[line];
	offset = BLine->offset;
	G_debug ( 3, "  line[%d] = %d, offset = %ld", j, line, offset );
	type = Vect_read_line (Map, Points, NULL, line );
	if ( lines[j] > 0 ) direction = GV_FORWARD; 
	else direction = GV_BACKWARD;
	Vect_append_points ( APoints, Points, direction);
    }
    
    dig_find_area_poly (APoints, &area_size);
    G_debug ( 3, "  area/isle size = %f", area_size );
    
    if (area_size > 0) {  /* area */
	/* add area structure to plus */
	area = dig_add_area (plus, n_lines, lines);
	if ( area == -1 ) { /* error */
	    Vect_close ( Map );
	    G_fatal_error ( _("Unable to add area (map closed, topo saved)") );
	}
	G_debug ( 3, "  -> area %d", area );
	return area;
    } else if (area_size < 0) { /* island */
	isle = dig_add_isle (plus, n_lines, lines);
	if ( isle == -1 ) { /* error */
	    Vect_close ( Map );
	    G_fatal_error ( _("Unable to add isle (map closed, topo saved)") );
	}
	G_debug ( 3, "  -> isle %d", isle );
	return -isle;
    } else { 
	/* TODO: What to do with such areas? Should be areas/isles of size 0 stored,
	*        so that may be found and cleaned by some utility
	*  Note: it would be useful for vertical closed polygons, but such would be added twice
	*        as area */
	G_warning (_("Area of size = 0.0 ignored")); 
    }
    return 0;
}

/*!
  \brief Find area outside island

  \param Map_info vector map
  \param isle isle id

  \return number of  area(s)
  \return 0 if not found
*/
int
Vect_isle_find_area ( struct Map_info *Map, int isle ) 
{
    int    j, line, node, sel_area, first, area, poly;
    static int first_call = 1;
    struct Plus_head *plus ;
    P_LINE *Line;
    P_NODE *Node;
    P_ISLE *Isle;
    P_AREA *Area;
    double  size, cur_size;
    BOUND_BOX box, abox;
    static struct ilist *List;
    static struct line_pnts *APoints;

    /* Note: We should check all isle points (at least) because if topology is not clean
     * and two areas overlap, isle which is not completely within area may be attached,
     * but it would take long time */

    G_debug ( 3, "Vect_isle_find_area () island = %d", isle );
    plus = &(Map->plus);

    if (  plus->Isle[isle] == NULL ) {
	G_warning (_("Request to find area outside nonexisting isle"));
	return 0;
    }

    if ( first_call ) {
	List = Vect_new_list ();
	APoints = Vect_new_line_struct ();
	first_call = 0;
    }

    Isle = plus->Isle[isle];
    line = abs(Isle->lines[0]);
    Line = plus->Line[line];
    node = Line->N1;
    Node = plus->Node[node];
    
    /* select areas by box */
    box.E = Node->x; box.W = Node->x; box.N = Node->y; box.S = Node->y;
    box.T = PORT_DOUBLE_MAX; box.B = -PORT_DOUBLE_MAX;
    Vect_select_areas_by_box (Map, &box, List);
    G_debug ( 3, "%d areas overlap island boundary point", List->n_values );
   
    sel_area = 0; cur_size = -1;
    first = 1;
    Vect_get_isle_box ( Map, isle, &box);
    for (j = 0; j < List->n_values; j++) {
	area = List->value[j];
	G_debug ( 3, "area = %d", area );

	Area = plus->Area[area];
	
        /* Before other tests, simply exclude those areas inside isolated isles formed by one boundary */
	if ( abs(Isle->lines[0]) == abs(Area->lines[0]) ) {
	    G_debug ( 3, "  area inside isolated isle" );
	    continue;
	}
	
	/* Check box */
	/* Note: If build is run on large files of areas imported from nontopo format (shapefile)
	 * attaching of isles takes very  long time because each area is also isle and select by
	 * box all overlaping areas selects all areas with box overlaping first node. 
	 * Then reading coordinates for all those areas would take a long time -> check first 
	 * if isle's box is completely within area box */
	Vect_get_area_box ( Map, area, &abox);
	if ( box.E > abox.E || box.W < abox.W || box.N > abox.N || box.S < abox.S ) {
	    G_debug ( 3, "  isle not completely inside area box" );
	    continue;
	}
	
	poly = Vect_point_in_area_outer_ring ( Node->x, Node->y, Map, area);
	G_debug ( 3, "  poly = %d", poly );
	
	if ( poly == 1) { /* pint in area, but node is not part of area inside isle (would be poly == 2) */
	    /* In rare case island is inside more areas in that case we have to calculate area
	     * of outer ring and take the smaller */
	    if ( sel_area == 0 ) { /* first */
		sel_area = area;
	    } else { /* is not first */
		if ( cur_size < 0 ) { /* second area */
		    /* This is slow, but should not be called often */
		    Vect_get_area_points (Map, sel_area, APoints);
		    G_begin_polygon_area_calculations();
		    cur_size = G_area_of_polygon(APoints->x, APoints->y, APoints->n_points);
	            G_debug ( 3, "  first area size = %f (n points = %d)", cur_size, APoints->n_points );

		} 

		Vect_get_area_points (Map, area, APoints);
		size = G_area_of_polygon(APoints->x, APoints->y, APoints->n_points);
	        G_debug ( 3, "  area size = %f (n points = %d)", cur_size, APoints->n_points );

		if ( size < cur_size ) {
		    sel_area = area;
		    cur_size = size;
		}
	    }
	    G_debug ( 3, "sel_area = %d cur_size = %f", sel_area, cur_size );
	}
    }
    if ( sel_area > 0 ) {	
	G_debug ( 3, "Island %d in area %d", isle, sel_area );
    } else { 
	G_debug ( 3, "Island %d is not in area", isle );
    }

    return sel_area;
}

/*!
  \brief (Re)Attach isle to area

  \param Map_info vector map
  \param isle isle id

  \return 0
*/
int
Vect_attach_isle ( struct Map_info *Map, int isle ) 
{
    int sel_area;
    P_ISLE *Isle;
    struct Plus_head *plus ;

    /* Note!: If topology is not clean and areas overlap, one island may fall to more areas
    *  (partially or fully). Before isle is attached to area it must be check if it is not attached yet */
    G_debug ( 3, "Vect_attach_isle (): isle = %d", isle);

    plus = &(Map->plus);
    
    sel_area = Vect_isle_find_area ( Map, isle );
    G_debug ( 3, "      isle = %d -> area outside = %d", isle, sel_area);
    if ( sel_area > 0 ) {
	Isle = plus->Isle[isle];
	if ( Isle->area > 0 ) {
	    G_debug (3, "Attempt to attach isle %d to more areas (=>topology is not clean)", isle);
	} else {
	    Isle->area = sel_area;
	    dig_area_add_isle ( plus, sel_area, isle );
	}
    }
    return 0;
}

/*!
  \brief (Re)Attach isles to areas in given box

  \param Map_info vector map
  \param box bounding box

  \return 0
*/
int
Vect_attach_isles ( struct Map_info *Map, BOUND_BOX *box )
{
    int i, isle;
    static int first = 1;
    static struct ilist *List;
    struct Plus_head *plus ;

    G_debug ( 3, "Vect_attach_isles ()");

    plus = &(Map->plus);

    if ( first ) {
        List = Vect_new_list (); 
	first = 0;
    }
    
    Vect_select_isles_by_box ( Map, box, List);
    G_debug ( 3, "  number of isles to attach = %d", List->n_values);

    for (i = 0; i < List->n_values; i++) { 
	isle = List->value[i];
        Vect_attach_isle ( Map, isle );
    }
    return 0;
}

/*!
  \brief (Re)Attach centroids to areas in given box

  \param Map_info vector map
  \param box bounding box

  \return 0
*/
int
Vect_attach_centroids ( struct Map_info *Map, BOUND_BOX *box ) 
{
    int i, sel_area, centr;
    static int first = 1;
    static struct ilist *List;
    P_AREA *Area;
    P_NODE *Node;
    P_LINE *Line;
    struct Plus_head *plus ;

    G_debug ( 3, "Vect_attach_centroids ()");
    
    plus = &(Map->plus);

    if ( first ) {
        List = Vect_new_list (); 
	first = 0;
    }

    /* Warning: If map is updated on level2, it may happen that previously correct island 
     * becomes incorrect. In that case, centroid of area forming the island is reattached 
     * to outer area, because island polygon is not excluded. 
     *
     * +-----------+     +-----------+
     * |   1       |     |   1       |
     * | +---+---+ |     | +---+---+ |     
     * | | 2 | 3 | |     | | 2 |     |   
     * | | x |   | |  -> | | x |     |  
     * | |   |   | |     | |   |     | 
     * | +---+---+ |     | +---+---+ |
     * |           |     |           |
     * +-----------+     +-----------+
     * centroid is       centroid is
     * attached to 2     reattached to 1
     *
     * Because of this, when the centroid is reattached to another area, it is always necessary
     * to check if original area exist, unregister centroid from previous area.
     * To simplify code, this is implemented so that centroid is always firs unregistered 
     * and if new area is found, it is registered again.
     */
    
    Vect_select_lines_by_box ( Map, box, GV_CENTROID, List);
    G_debug ( 3, "  number of centroids to reattach = %d", List->n_values);
    for (i = 0; i < List->n_values; i++) { 
	int orig_area;
	centr = List->value[i];
	Line = plus->Line[centr];
	Node = plus->Node[Line->N1];

	/* Unregister centroid */
	orig_area = Line->left;
	if ( orig_area > 0 ) {
	    if ( plus->Area[orig_area] != NULL ) {
		plus->Area[orig_area]->centroid = 0;
	    }
	}
	Line->left = 0;
	
	sel_area = Vect_find_area ( Map, Node->x, Node->y );
	G_debug ( 3, "  centroid %d is in area %d", centr, sel_area);
	if ( sel_area > 0 ) {
	    Area = plus->Area[sel_area];
	    if ( Area->centroid == 0 ) { /* first centroid */
		G_debug ( 3, "  first centroid -> attach to area");
		Area->centroid = centr;
		Line->left = sel_area;
	    } else if ( Area->centroid != centr ) {  /* duplicate centroid */
		/* Note: it cannot happen that Area->centroid == centr, because the centroid
		 * was previously unregistered */
		G_debug ( 3, "  duplicate centroid -> do not attach to area");
		Line->left = -sel_area;
	    }
	}
	    
	if ( sel_area != orig_area && plus->do_uplist ) dig_line_add_updated ( plus, centr );
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
Vect_build_nat ( struct Map_info *Map, int build, FILE *msgout )
{
    struct Plus_head *plus ;
    int    i, j, s, type, lineid;
    long offset; 
    int    side, line, area;
    struct line_pnts *Points, *APoints;
    struct line_cats *Cats;
    P_LINE *Line;
    P_NODE *Node;
    P_AREA *Area;
    BOUND_BOX box;
    struct ilist *List;

    G_debug (3, "Vect_build_nat() build = %d", build);
    
    plus = &(Map->plus);
    Msgout = msgout;
    
    if ( build == plus->built ) return 1; /* Do nothing */
    
    /* Check if upgrade or downgrade */
    if ( build < plus->built ) { /* lower level request */

	/* release old sources (this also initializes structures and numbers of elements) */
	if ( plus->built >= GV_BUILD_CENTROIDS && build < GV_BUILD_CENTROIDS) {
	    /* reset info about areas stored for centroids */
	    int nlines = Vect_get_num_lines (Map);
	    for ( line = 1; line <= nlines; line++ ) {
		Line = plus->Line[line];
		if ( Line && Line->type == GV_CENTROID ) 
		    Line->left = 0;
	    }
	    dig_free_plus_areas (plus);
	    dig_spidx_free_areas (plus);
	    dig_free_plus_isles (plus);
	    dig_spidx_free_isles (plus);
	}


	if ( plus->built >= GV_BUILD_AREAS && build < GV_BUILD_AREAS) {
	    /* reset info about areas stored for lines */
	    int nlines = Vect_get_num_lines (Map);
	    for ( line = 1; line <= nlines; line++ ) {
		Line = plus->Line[line];
		if ( Line && Line->type == GV_BOUNDARY ) {
		    Line->left = 0;
		    Line->right = 0;
		}
	    }
	    dig_free_plus_areas (plus);
	    dig_spidx_free_areas (plus);
	    dig_free_plus_isles (plus);
	    dig_spidx_free_isles (plus);
	}
	if ( plus->built >= GV_BUILD_BASE && build < GV_BUILD_BASE) {
	    dig_free_plus_nodes (plus);
	    dig_spidx_free_nodes (plus);
	    dig_free_plus_lines (plus);
	    dig_spidx_free_lines (plus);
	}
	
	plus->built = build;
	return 1;
    }
    
    Points = Vect_new_line_struct ();
    APoints = Vect_new_line_struct ();
    Cats = Vect_new_cats_struct ();
    List = Vect_new_list ();

    if ( plus->built < GV_BUILD_BASE ) { 
	/* 
	*  We shall go through all primitives in coor file and 
	*  add new node for each end point to nodes structure
	*  if the node with the same coordinates doesn't exist yet.
	*/
       
	/* register lines, create nodes */ 
	Vect_rewind ( Map );
	prnmsg (_("Registering lines: "));
	i = 1; j = 1;
	while ( 1 ) {
	    /* register line */
	    type = Vect_read_next_line (Map, Points, Cats);
	    /* Note: check for dead lines is not needed, because they are skipped by V1_read_next_line_nat() */
	    if ( type == -1 ) { 
		G_warning(_("Unable to read vector map"));
		return 0;
	    } else if ( type == -2 ) {
		break;
	    }
	    
	    offset = Map->head.last_offset;

	    G_debug ( 3, "Register line: offset = %ld", offset );
	    lineid = dig_add_line ( plus, type, Points, offset );
	    dig_line_box ( Points, &box );
	    if ( lineid == 1 ) 
		Vect_box_copy (&(plus->box), &box);
	    else
		Vect_box_extend (&(plus->box), &box);

	    /* Add all categories to category index */
	    if ( build == GV_BUILD_ALL ) {
		int c; 

		for ( c = 0; c < Cats->n_cats; c++ ) {
		    dig_cidx_add_cat (plus, Cats->field[c], Cats->cat[c], lineid, type);
		}
		if ( Cats->n_cats == 0 ) /* add field 0, cat 0 */
		    dig_cidx_add_cat (plus, 0, 0, lineid, type);
	    }
	    
	    /* print progress */
	    if ( i == 1000 ) {
		prnmsg ("%7d\b\b\b\b\b\b\b", j);
		i = 0;
	    }
	    i++; j++;
	}
	prnmsg ("\r%d %s      \n", plus->n_lines, _("primitives registered"));

	plus->built = GV_BUILD_BASE;
    }

    if ( build < GV_BUILD_AREAS ) return 1;

    if ( plus->built < GV_BUILD_AREAS ) {
	/* Build areas */
	/* Go through all bundaries and try to build area for both sides */
	prnmsg (_("Building areas: "));
	for (i = 1; i <= plus->n_lines; i++) {
	    G_percent2 ( i, plus->n_lines, 1, msgout );

	    /* build */
	    if ( plus->Line[i] == NULL ) { continue; } /* dead line */
	    Line = plus->Line[i];
	    if ( Line->type != GV_BOUNDARY ) { continue; }

	    for (s = 0; s < 2; s++) {
		if ( s == 0 ) side = GV_LEFT;
		else side = GV_RIGHT;
	       
		G_debug ( 3, "Build area for line = %d, side = %d", i, side );
		Vect_build_line_area ( Map, i, side );
	    }
	}
	prnmsg ("\r%d %s      \n%d %s\n", plus->n_areas, _("areas built"), plus->n_isles, _("isles built"));
	plus->built = GV_BUILD_AREAS;
    }
    
    if ( build < GV_BUILD_ATTACH_ISLES ) return 1;
    
    /* Attach isles to areas */
    if ( plus->built < GV_BUILD_ATTACH_ISLES ) {
	prnmsg (_("Attaching islands: "));
	for (i = 1; i <= plus->n_isles; i++) {
	    G_percent2 ( i, plus->n_isles, 1, msgout );
	    Vect_attach_isle ( Map, i ) ;
	}
	if (i==1) prnmsg ("\n");
	plus->built = GV_BUILD_ATTACH_ISLES;
    }
    
    if ( build < GV_BUILD_CENTROIDS ) return 1;

    /* Attach centroids to areas */
    if ( plus->built < GV_BUILD_CENTROIDS ) {
	int nlines;
	
	prnmsg (_("Attaching centroids: "));
	
	nlines = Vect_get_num_lines (Map);
	for ( line = 1; line <= nlines; line++ ) {
	    G_percent2 ( line, nlines, 1, msgout );

	    Line = plus->Line[line];
	    if ( !Line ) continue; /* Dead */

	    if ( Line->type != GV_CENTROID ) continue;

	    Node = plus->Node[Line->N1];

	    area = Vect_find_area ( Map, Node->x, Node->y );

	    if ( area > 0 ) {
		G_debug ( 3, "Centroid (line=%d) in area %d", line, area );

		Area =  plus->Area[area];

		if ( Area->centroid == 0 ) { /* first */
		    Area->centroid = line;
		    Line->left = area;
		} else { /* duplicate */
		    Line->left = -area;
		}
	    }
	}
	plus->built = GV_BUILD_CENTROIDS;
    }

    /* Add areas to category index */
    for ( area = 1; area <= plus->n_areas; area++ ) {
	int c;

	if ( plus->Area[area] == NULL ) continue;

	if ( plus->Area[area]->centroid > 0 ) { 
	    Vect_read_line (Map, NULL, Cats, plus->Area[area]->centroid );

	    for ( c = 0; c < Cats->n_cats; c++ ) {
		dig_cidx_add_cat (plus, Cats->field[c], Cats->cat[c], area, GV_AREA);
	    }
	}

	if ( plus->Area[area]->centroid == 0 || Cats->n_cats == 0 ) /* no centroid or no cats */
            dig_cidx_add_cat (plus, 0, 0, area, GV_AREA);
    }

    return 1;
}
