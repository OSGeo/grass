#include <string.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "plot.h"

int attr ( struct Map_info *Map, int type, char *attrcol, 
           struct cat_list *Clist, LATTR *lattr, int chcat)
{
    int i, ltype, more;
    double xl, yl;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int X, Y, T, B, L, R, Xoffset, Yoffset, xarr[5], yarr[5];
    int cat;
    char buf[2000];
    struct field_info *fi;
    dbDriver *driver;
    dbString stmt, valstr, text;
    dbCursor cursor;
    dbTable  *table;
    dbColumn *column;
    
    G_debug (2, "attr()");

    if ( attrcol == NULL || *attrcol == '\0' ) {
	G_fatal_error (_("attrcol not specified, cannot display attributes"));
    }
    
    Points = Vect_new_line_struct ();
    Cats = Vect_new_cats_struct ();
    db_init_string (&stmt);
    db_init_string (&valstr);
    db_init_string (&text);

    fi = Vect_get_field(Map, lattr->field);
    if ( fi == NULL ) return 1;

    driver = db_start_driver_open_database ( fi->driver, fi->database );
    if ( driver == NULL )
	G_fatal_error (_("Cannot open database %s by driver %s"), fi->database, fi->driver );

    Vect_rewind ( Map );
    while (1)
    {
	ltype =  Vect_read_next_line (Map, Points, Cats);   
        switch ( ltype )
	{
	case -1:
	    G_fatal_error (_("Can't read vector map"));
	case -2: /* EOF */
	    return  0;
	}

	if ( !(type & ltype) ) continue; /* used for both lines and labels */
	
	R_RGB_color(lattr->color.R, lattr->color.G, lattr->color.B) ;
	R_text_size(lattr->size, lattr->size) ;
	if (lattr->font)
	    R_font(lattr->font) ;
		  
        if ( chcat ) {
	     int found = 0;

	     for ( i = 0; i < Cats->n_cats; i++ ) {
		 if ( Cats->field[i] == Clist->field && Vect_cat_in_cat_list ( Cats->cat[i], Clist) ) {
		     found = 1;
		     break;
		 }
	     }
	     if (!found) continue;
        } else if ( Clist->field > 0 ) {
	     int found = 0;

	     for ( i = 0; i < Cats->n_cats; i++ ) {
		 if ( Cats->field[i] == Clist->field ) {
		     found = 1;
		     break;
		 }
	     }
	     /* lines with no category will be displayed */
	     if (Cats->n_cats > 0 && !found) continue;
	}

	if( Vect_cat_get(Cats, lattr->field, &cat) )
	  {	    
	    int ncats = 0;
	    /* Read attribute from db */
	    db_free_string (&text);
	    for ( i = 0; i < Cats->n_cats; i++ ) {
		int nrows;
		
		if ( Cats->field[i] != lattr->field ) continue;
		db_init_string (&stmt);
		sprintf (buf, "select %s from %s where %s = %d", attrcol, fi->table, fi->key, Cats->cat[i]);
		G_debug (2, "SQL: %s", buf);
		db_append_string ( &stmt, buf);   
		
		if (db_open_select_cursor(driver, &stmt, &cursor, DB_SEQUENTIAL) != DB_OK)
		    G_fatal_error (_("Cannot select attributes: %s"), db_get_string(&stmt) );

		nrows = db_get_num_rows ( &cursor );

		if (ncats > 0) 
		    db_append_string (&text, "/");

		if ( nrows > 0 ) {
		    table = db_get_cursor_table (&cursor);
		    column = db_get_table_column(table, 0); /* first column */
		    
		    if(db_fetch (&cursor, DB_NEXT, &more) != DB_OK) continue;
		    db_convert_column_value_to_string (column, &valstr); 

		    db_append_string (&text, db_get_string(&valstr));
		} else {
		    G_warning (_("No attribute found for cat %d: %s"), cat, db_get_string(&stmt));
		}

		db_close_cursor(&cursor);
		ncats++;
	    }
	    
	    if ( (ltype & GV_POINTS) || Points->n_points == 1 )
	      /* point/centroid or line/boundary with one coor */     
	      {
	        X = (int)(D_u_to_d_col(Points->x[0])) ;
	        Y = (int)(D_u_to_d_row(Points->y[0])) ;
	      }
	    else if ( Points->n_points == 2 ) /* line with two coors */
	      {
                xl = (Points->x[0] + Points->x[1]) / 2;
                yl = (Points->y[0] + Points->y[1]) / 2;
	        X = (int)(D_u_to_d_col(xl)) ;
	        Y = (int)(D_u_to_d_row(yl)) ;
	      }
	    else
	      {
                i = Points->n_points / 2;   
	        X = (int)(D_u_to_d_col(Points->x[i])) ;
	        Y = (int)(D_u_to_d_row(Points->y[i])) ;
	      }
	   
            X = X + 0.5 * lattr->size;
            Y = Y + 1.5 * lattr->size;
	
            R_move_abs(X, Y) ;
            R_get_text_box(db_get_string (&text), &T, &B, &L, &R);
		
            /* Expand border 1/2 of text size */
	    T = T - lattr->size / 2 ;
	    B = B + lattr->size / 2 ;
	    L = L - lattr->size / 2 ;
            R = R + lattr->size / 2 ;
               
	    Xoffset = 0;
	    Yoffset = 0;
	    if (lattr->xref == LCENTER) Xoffset = -(R - L) / 2 ;
	    if (lattr->xref == LRIGHT ) Xoffset = -(R - L) ;
            if (lattr->yref == LCENTER) Yoffset = -(B - T) / 2 ;
	    if (lattr->yref == LBOTTOM) Yoffset = -(B - T) ; 
		
	    if ( lattr->has_bgcolor || lattr->has_bcolor )
	      {
	        xarr[0] = xarr[1] = xarr[4] = L + Xoffset; 
	        xarr[2] = xarr[3] = R + Xoffset; 
	        yarr[0] = yarr[3] = yarr[4] = B + Yoffset; 
	        yarr[1] = yarr[2] = T + Yoffset; 
		
                if( lattr->has_bgcolor)
                  {
	             R_RGB_color( lattr->bgcolor.R, lattr->bgcolor.G, lattr->bgcolor.B) ;
		     R_polygon_abs(xarr, yarr, 5) ;
                  }
		
                if( lattr->has_bcolor)
	          {
	             R_RGB_color( lattr->bcolor.R, lattr->bcolor.G, lattr->bcolor.B) ;
		     R_polyline_abs(xarr, yarr, 5) ;
	          }
	        R_RGB_color( lattr->color.R, lattr->color.G, lattr->color.B) ;
	      }
		
	    R_move_abs(X + Xoffset, Y + Yoffset) ;
  	    R_text(db_get_string (&text));
	}
    }

    db_close_database_shutdown_driver ( driver );
    Vect_destroy_line_struct (Points);
    Vect_destroy_cats_struct (Cats);
    
    return 0;
}
