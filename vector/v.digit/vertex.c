#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/form.h>
#include "global.h"
#include "proto.h"

/* Split line */
struct split_line
{
    struct line_pnts *Points, *NPoints;
    struct line_cats *Cats;
    int last_line, last_seg;
    double thresh;
    double xo, yo;
};

int split_line_begin(void *closure)
{
    struct split_line *sl = closure;

    G_debug (2, "split_line()");

    sl->Points = Vect_new_line_struct ();
    sl->NPoints = Vect_new_line_struct ();
    sl->Cats = Vect_new_cats_struct ();
    
    i_prompt ( "Split line:"); 
    i_prompt_buttons ( "Select", "", "Quit tool"); 
    
    /* TODO: use some better threshold */
    sl->thresh = fabs ( D_d_to_u_col ( 10 ) - D_d_to_u_col ( 0 ) ) ; 
    G_debug (2, "thresh = %f", sl->thresh );
    
    sl->last_line = 0;
    sl->last_seg = 0;

    set_mode(MOUSE_POINT);

    return 0;
}

int split_line_update(void *closure, int sxn, int syn, int button)
{
    struct split_line *sl = closure;
    double x =  D_d_to_u_col ( sxn );
    double y =  D_d_to_u_row ( syn );

    if ( sl->last_line == 0 ) {
	i_prompt_buttons ( "Select", "", "Quit tool"); 
    } 
	
    if ( sl->last_line > 0 ) {
	display_line ( sl->last_line, SYMB_DEFAULT, 1);
    }

    G_debug (3, "button = %d x = %d = %f y = %d = %f", button, sxn, x, syn, y);

    if ( button == 3 ) return 1;

    if ( button == 1 ) { /* Select / split */
	int line;
	if ( sl->last_line > 0 ) { /* Line is already selected -> split */
	    int node1, node2, type, np, i;
	    display_line ( sl->last_line, SYMB_BACKGROUND, 1);
	    Vect_get_line_nodes ( &Map, sl->last_line, &node1, &node2 ); 
	    display_node ( node1, SYMB_BACKGROUND, 1);
	    display_node ( node2, SYMB_BACKGROUND, 1);
	    symb_set_driver_color ( SYMB_BACKGROUND );
	    display_icon ( sl->xo, sl->yo, G_ICON_CROSS, 0, 10, 1);

	    /* Read and delete old */
	    type = Vect_read_line ( &Map, sl->Points, sl->Cats, sl->last_line );
	    Vect_delete_line ( &Map, sl->last_line );
	    updated_lines_and_nodes_erase_refresh_display();
	    np = sl->Points->n_points;

	    /* First part */
	    Vect_reset_line ( sl->NPoints );
	    for ( i = 0; i < sl->last_seg; i++ ){ 
		Vect_append_point (sl->NPoints, sl->Points->x[i], sl->Points->y[i], sl->Points->z[i] );
	    }
	    Vect_append_point (sl->NPoints, sl->xo, sl->yo, 0 );
	    Vect_write_line( &Map, type, sl->NPoints, sl->Cats);
	    updated_lines_and_nodes_erase_refresh_display();

	    /* Second part */
	    Vect_reset_line ( sl->NPoints );
	    Vect_append_point (sl->NPoints, sl->xo, sl->yo, 0 );
	    for ( i = sl->last_seg; i < np; i++ ){ 
		Vect_append_point (sl->NPoints, sl->Points->x[i], sl->Points->y[i], sl->Points->z[i] );
	    }
	    Vect_write_line( &Map, type, sl->NPoints, sl->Cats);
	    updated_lines_and_nodes_erase_refresh_display();

	    sl->last_line = 0;
	} 

	/* Select vertex */ 
	line = Vect_find_line (&Map, x, y, 0, GV_LINE|GV_BOUNDARY, sl->thresh, 0, 0);
	G_debug (2, "line found = %d", line );
	    
	/* Display new selected line if any */
	if ( line > 0 ) {
	    int seg;
	    /* Find the nearest vertex on the line */
	    Vect_read_line ( &Map, sl->Points, NULL, line );
	    seg = Vect_line_distance ( sl->Points, x, y, 0, 0, &sl->xo, &sl->yo, NULL, NULL, NULL, NULL );

	    display_line ( line, SYMB_HIGHLIGHT, 1);
	    symb_set_driver_color ( SYMB_HIGHLIGHT );
	    display_icon ( sl->xo, sl->yo, G_ICON_CROSS, 0, 10, 1);

	    i_prompt_buttons ( "Confirm and select next", "Unselect", "Quit tool"); 
	    sl->last_line = line;
	    sl->last_seg = seg;
	}
    }
    if ( button == 2 ) { /* Unselect */
	if ( sl->last_line > 0 ) {
	    symb_set_driver_color ( SYMB_BACKGROUND );
	    display_icon ( sl->xo, sl->yo, G_ICON_CROSS, 0, 10, 1);
	    sl->last_line = 0;
	}
    }

    return 0;
}

int split_line_end(void *closure)
{
    struct split_line *sl = closure;

    if ( sl->last_line == 0 ) {
	i_prompt_buttons ( "Select", "", "Quit tool"); 
    } 
	
    if ( sl->last_line > 0 ) {
	display_line ( sl->last_line, SYMB_DEFAULT, 1);
    }
    
    if ( sl->last_line > 0 ) {
	symb_set_driver_color ( SYMB_BACKGROUND );
	display_icon ( sl->xo, sl->yo, G_ICON_CROSS, 0, 10, 1);
    }

    i_prompt (""); 
    i_prompt_buttons ( "", "", ""); 
    i_coor ( COOR_NULL, COOR_NULL); 
    
    G_debug (3, "split_line(): End");

    return 1;
}

void split_line(void)
{
    static struct split_line sl;

    set_tool(split_line_begin, split_line_update, split_line_end, &sl);
}

/* Remove line vertex */
struct rm_vertex
{
    struct line_pnts *Points;
    struct line_cats *Cats;
    int last_line, last_seg;
    double thresh;
    double xo, yo;
};

int rm_vertex_begin(void *closure)
{
    struct rm_vertex *rv = closure;

    G_debug (2, "remove_vertex()");

    rv->Points = Vect_new_line_struct ();
    rv->Cats = Vect_new_cats_struct ();
    
    i_prompt ( "Remove vertex:"); 
    i_prompt_buttons ( "Select vertex", "", "Quit tool"); 
    
    /* TODO: use some better threshold */
    rv->thresh = fabs ( D_d_to_u_col ( 10 ) - D_d_to_u_col ( 0 ) ) ; 
    G_debug (2, "thresh = %f", rv->thresh );
    
    rv->last_line = 0;
    rv->last_seg = 0;

    set_mode(MOUSE_POINT);

    return 0;
}

int rm_vertex_update(void *closure, int sxn, int syn, int button)
{
    struct rm_vertex *rv = closure;
    double x =  D_d_to_u_col ( sxn );
    double y =  D_d_to_u_row ( syn );

    if ( rv->last_line == 0 ) {
	i_prompt_buttons ( "Select vertex", "", "Quit tool"); 
    } 
	
    if ( rv->last_line > 0 ) {
	display_line ( rv->last_line, SYMB_DEFAULT, 1);
    }

    G_debug (3, "button = %d x = %d = %f y = %d = %f", button, sxn, x, syn, y);

    if ( button == 3 ) return 1;

    if ( button == 1 ) { /* Select / new location */
	int line;
	if ( rv->last_line > 0 ) { /* Line is already selected */
	    int node1, node2, type, np, i;
	    display_line ( rv->last_line, SYMB_BACKGROUND, 1);
	    Vect_get_line_nodes ( &Map, rv->last_line, &node1, &node2 ); 
	    display_node ( node1, SYMB_BACKGROUND, 1);
	    display_node ( node2, SYMB_BACKGROUND, 1);
	    symb_set_driver_color ( SYMB_BACKGROUND );
	    display_icon ( rv->xo, rv->yo, G_ICON_BOX, 0, 10, 1);

	    type = Vect_read_line ( &Map, rv->Points, rv->Cats, rv->last_line );
	    np = rv->Points->n_points;
	    for ( i = rv->last_seg; i < np - 1; i++ ){ 
		rv->Points->x[i] = rv->Points->x[i+1];
		rv->Points->y[i] = rv->Points->y[i+1];
		rv->Points->z[i] = rv->Points->z[i+1];
	    }
	    rv->Points->n_points--;
	    Vect_rewrite_line(&Map, rv->last_line, type, rv->Points, rv->Cats);
	    updated_lines_and_nodes_erase_refresh_display();
	    rv->last_line = 0;
	} 

	/* Select vertex */ 
	line = Vect_find_line (&Map, x, y, 0, GV_LINE|GV_BOUNDARY, rv->thresh, 0, 0);
	G_debug (2, "line found = %d", line );
	    
	/* Display new selected line if any */
	if ( line > 0 ) {
	    int seg;
	    double dist;
	    /* Find the nearest vertex on the line */
	    Vect_read_line ( &Map, rv->Points, NULL, line );
	    seg = Vect_line_distance ( rv->Points, x, y, 0, 0, &rv->xo, &rv->yo, NULL, NULL, NULL, NULL );

	    dist = Vect_points_distance ( rv->xo, rv->yo, 0, rv->Points->x[seg-1], rv->Points->y[seg-1], 0, 0);

	    if ( dist < Vect_points_distance ( rv->xo, rv->yo, 0, rv->Points->x[seg], rv->Points->y[seg], 0, 0) ) {
		seg -= 1;
	    }
		
	    rv->xo = rv->Points->x[seg];
	    rv->yo = rv->Points->y[seg];
		
	    display_line ( line, SYMB_HIGHLIGHT, 1);
	    symb_set_driver_color ( SYMB_HIGHLIGHT );
	    display_icon ( rv->xo, rv->yo, G_ICON_BOX, 0, 10, 1);

	    i_prompt_buttons ( "Confirm and select next", "Unselect", "Quit tool"); 
	    rv->last_line = line;
	    rv->last_seg = seg;
	}
    }
    if ( button == 2 ) { /* Unselect */
	if ( rv->last_line > 0 ) {
	    symb_set_driver_color ( SYMB_BACKGROUND );
	    display_icon ( rv->xo, rv->yo, G_ICON_BOX, 0, 10, 1);
	    rv->last_line = 0;
	}
    }

    return 0;
}

int rm_vertex_end(void *closure)
{
    struct rm_vertex *rv = closure;

    if ( rv->last_line == 0 ) {
	i_prompt_buttons ( "Select vertex", "", "Quit tool"); 
    } 
	
    if ( rv->last_line > 0 ) {
	display_line ( rv->last_line, SYMB_DEFAULT, 1);
    }

    if ( rv->last_line > 0 ) {
	symb_set_driver_color ( SYMB_BACKGROUND );
	display_icon ( rv->xo, rv->yo, G_ICON_BOX, 0, 10, 1);
    }
    
    i_prompt (""); 
    i_prompt_buttons ( "", "", ""); 
    i_coor ( COOR_NULL, COOR_NULL); 
    
    G_debug (3, "remove_vertex(): End");

    return 1;
}

void rm_vertex(void)
{
    static struct rm_vertex rv;

    set_tool(rm_vertex_begin, rm_vertex_update, rm_vertex_end, &rv);
}

/* Add new vertex to line */
struct add_vertex
{
    struct line_pnts *Points;
    struct line_cats *Cats;
    int last_line, last_seg;
    int do_snap;
    double thresh;
};

int add_vertex_begin(void *closure)
{
    struct add_vertex *av = closure;

    G_debug (2, "add_vertex()");

    av->Points = Vect_new_line_struct ();
    av->Cats = Vect_new_cats_struct ();
    
    i_prompt ( "Add vertex:"); 
    i_prompt_buttons ( "Select", "", "Quit tool"); 
    
    /* TODO: use some better threshold */
    av->thresh = fabs ( D_d_to_u_col ( 10 ) - D_d_to_u_col ( 0 ) ) ; 
    G_debug (2, "thresh = %f", av->thresh );
    
    av->last_line = 0;
    av->last_seg = 0;
    av->do_snap = 0;

    set_mode(MOUSE_POINT);

    return 0;
}

int add_vertex_update(void *closure, int sxn, int syn, int button)
{
    struct add_vertex *av = closure;
    double x =  D_d_to_u_col ( sxn );
    double y =  D_d_to_u_row ( syn );

    G_debug (3, "button = %d x = %d = %f y = %d = %f", button, sxn, x, syn, y);

    if ( button == 3 ) return 1;
	
    if ( av->last_line > 0 ) {
	display_line ( av->last_line, SYMB_DEFAULT, 1);
    }

    if ( button == 1 ) { /* Select line segment */
	if ( av->last_line == 0 ) { /* Select line */ 
	    int line = Vect_find_line (&Map, x, y, 0, GV_LINE|GV_BOUNDARY, av->thresh, 0, 0);
	    G_debug (2, "line found = %d", line );
                
	    /* Display new selected line if any */
	    if ( line > 0 ) {
		int seg, len;
		double xo, yo, px, py;
		double dist;
		display_line ( line, SYMB_HIGHLIGHT, 1);

		/* Find the nearest vertex on the line */
		Vect_read_line ( &Map, av->Points, NULL, line );
		seg = Vect_line_distance ( av->Points, x, y, 0, 0, &px, &py, NULL, NULL, NULL, NULL );

		G_debug (3, "seg = %d", seg );

		xo = ( av->Points->x[seg-1] + av->Points->x[seg] ) / 2;
		yo = ( av->Points->y[seg-1] + av->Points->y[seg] ) / 2;

		/* If close to first or last point insert before / after the line. 
		 * 'close' is here < 1/4 of segment length */
		av->do_snap = 0;
		if ( seg == 1 ) {
		    dist = Vect_points_distance ( px, py, 0, av->Points->x[0], av->Points->y[0], 0, 0);
		    len = Vect_points_distance ( av->Points->x[0], av->Points->y[0], 0, 
						 av->Points->x[1], av->Points->y[1], 0, 0);
                    
		    if ( dist < len/4 ) {
			seg = 0;
			xo = av->Points->x[0]; 
			yo = av->Points->y[0]; 
			av->do_snap = 1;
		    }
		}

		if ( seg == av->Points->n_points - 1 ) {
		    int np = av->Points->n_points;
		    double dist = Vect_points_distance ( px, py, 0, av->Points->x[np-1], av->Points->y[np-1], 0, 0);
		    int len = Vect_points_distance  ( av->Points->x[np-2], av->Points->y[np-2], 0, 
						  av->Points->x[np-1], av->Points->y[np-1], 0, 0);
		    if ( dist < len/4 ) {
			seg ++;
			xo = av->Points->x[np-1]; 
			yo = av->Points->y[np-1]; 
			av->do_snap = 1;
		    }
		}
		G_debug (3, "seg 2 = %d", seg );

		set_location(D_u_to_d_col ( xo ), D_u_to_d_row ( yo ));

		i_prompt_buttons ( "New vertex", "Unselect", "Quit tool"); 
		av->last_line = line;
		av->last_seg = seg;
	    }
	} else { /* Line is already selected -> new vertex */
	    int node1, node2, type, np, i;
	    if ( av->do_snap ) {
		snap ( &x, &y );
	    }
	    display_line ( av->last_line, SYMB_BACKGROUND, 1);
	    Vect_get_line_nodes ( &Map, av->last_line, &node1, &node2 ); 
	    display_node ( node1, SYMB_BACKGROUND, 1);
	    display_node ( node2, SYMB_BACKGROUND, 1);

	    type = Vect_read_line ( &Map, av->Points, av->Cats, av->last_line );
	    np = av->Points->n_points;
	    /* insert vertex */
	    Vect_append_point (av->Points, 0, 0, 0);
	    for ( i = np; i > av->last_seg; i-- ) {
		av->Points->x[i] = av->Points->x[i-1];
		av->Points->y[i] = av->Points->y[i-1];
		av->Points->z[i] = av->Points->z[i-1];
	    }
		
	    av->Points->x[av->last_seg] = x;
	    av->Points->y[av->last_seg] = y;
	    av->Points->z[av->last_seg] = 0;
		
	    Vect_rewrite_line(&Map, av->last_line, type, av->Points, av->Cats);
	    updated_lines_and_nodes_erase_refresh_display();
	    av->last_line = 0;
	}

    }

    if ( button == 2 ) { /* Unselect */
	if ( av->last_line > 0 ) {
	    av->last_line = 0;
	}
    }

    if ( av->last_line == 0 )
    {
	i_prompt_buttons ( "Select", "", "Quit tool"); 
	set_mode(MOUSE_POINT);
    }
    else
	set_mode(MOUSE_LINE);

    return 0;
}

int add_vertex_end(void *closure)
{
    struct add_vertex *av = closure;
	
    if ( av->last_line > 0 ) {
	display_line ( av->last_line, SYMB_DEFAULT, 1);
    }
    
    i_prompt (""); 
    i_prompt_buttons ( "", "", ""); 
    i_coor ( COOR_NULL, COOR_NULL); 
    
    G_debug (3, "add_vertex(): End");

    return 1;
}

void add_vertex(void)
{
    static struct add_vertex av;

    set_tool(add_vertex_begin, add_vertex_update, add_vertex_end, &av);
}

/* Move vertex */

struct move_vertex
{
    struct line_pnts *Points;
    struct line_cats *Cats;
    int last_line, last_seg;
    double thresh;
    double xo, yo;
};

int move_vertex_begin(void *closure)
{
    struct move_vertex *mv = closure;

    G_debug (2, "move_vertex()");

    mv->Points = Vect_new_line_struct ();
    mv->Cats = Vect_new_cats_struct ();
    
    i_prompt ( "Move vertex:"); 
    i_prompt_buttons ( "Select", "", "Quit tool"); 
    
    /* TODO: use some better threshold */
    mv->thresh = fabs ( D_d_to_u_col ( 10 ) - D_d_to_u_col ( 0 ) ) ; 
    G_debug (2, "thresh = %f", mv->thresh );
    
    mv->last_line = 0;
    mv->last_seg = 0;

    set_mode(MOUSE_POINT);

    return 0;
}

int move_vertex_update(void *closure, int sxn, int syn, int button)
{
    struct move_vertex *mv = closure;
    double x =  D_d_to_u_col ( sxn );
    double y =  D_d_to_u_row ( syn );

    G_debug (3, "button = %d x = %d = %f y = %d = %f", button, sxn, x, syn, y);
	
    if ( mv->last_line > 0 ) {
	display_line ( mv->last_line, SYMB_DEFAULT, 1);
    }

    if ( button == 3 ) return 1;

    if ( button == 1 ) { /* Select / new location */
	if ( mv->last_line == 0 ) { /* Select line */ 
	    int line = Vect_find_line (&Map, x, y, 0, GV_LINE|GV_BOUNDARY, mv->thresh, 0, 0);
	    G_debug (2, "line found = %d", line );
                
	    /* Display new selected line if any */
	    if ( line > 0 ) {
		int seg;
		double xo, yo, dist;
		display_line ( line, SYMB_HIGHLIGHT, 1);

		/* Find the nearest vertex on the line */
		Vect_read_line ( &Map, mv->Points, NULL, line );
		seg = Vect_line_distance ( mv->Points, x, y, 0, 0, &xo, &yo, NULL, NULL, NULL, NULL );

		dist = Vect_points_distance ( xo, yo, 0, mv->Points->x[seg-1], mv->Points->y[seg-1], 0, 0);

		if ( dist < Vect_points_distance ( xo, yo, 0, mv->Points->x[seg], mv->Points->y[seg], 0, 0) ) {
		    seg -= 1;
		}
		    
		mv->xo = mv->Points->x[seg];
		mv->yo = mv->Points->y[seg];
		set_location(D_u_to_d_col ( mv->xo  ), D_u_to_d_row ( mv->yo ));


		i_prompt_buttons ( "New location", "Unselect", "Quit tool"); 
		mv->last_line = line;
		mv->last_seg = seg;
	    }
	} else { /* Line is already selected */
	    int type, node1, node2;
	    if ( mv->last_seg == 0 || mv->last_seg == mv->Points->n_points - 1 ) {
		snap ( &x, &y );
	    }
	    display_line ( mv->last_line, SYMB_BACKGROUND, 1);
	    Vect_get_line_nodes ( &Map, mv->last_line, &node1, &node2 ); 
	    display_node ( node1, SYMB_BACKGROUND, 1);
	    display_node ( node2, SYMB_BACKGROUND, 1);

	    type = Vect_read_line ( &Map, mv->Points, mv->Cats, mv->last_line );
	    mv->Points->x[mv->last_seg] = mv->Points->x[mv->last_seg] + x - mv->xo;
	    mv->Points->y[mv->last_seg] = mv->Points->y[mv->last_seg] + y - mv->yo;
	    Vect_rewrite_line(&Map, mv->last_line, type, mv->Points, mv->Cats);
	    updated_lines_and_nodes_erase_refresh_display();
	    mv->last_line = 0;
	}

    }
    if ( button == 2 ) { /* Unselect */
	if ( mv->last_line > 0 ) {
	    mv->last_line = 0;
	}
    }


    if ( mv->last_line == 0 ) {
	i_prompt_buttons ( "Select", "", "Quit tool"); 
	set_mode(MOUSE_POINT);
    } else
	set_mode(MOUSE_LINE);

    return 0;
}

int move_vertex_end(void *closure)
{
    struct move_vertex *mv = closure;
	
    if ( mv->last_line > 0 ) {
	display_line ( mv->last_line, SYMB_DEFAULT, 1);
    }
    
    i_prompt (""); 
    i_prompt_buttons ( "", "", ""); 
    i_coor ( COOR_NULL, COOR_NULL); 
    
    G_debug (3, "move_vertex(): End");

    return 1;
}

void move_vertex(void)
{
    static struct move_vertex mv;

    set_tool(move_vertex_begin, move_vertex_update, move_vertex_end, &mv);
}

