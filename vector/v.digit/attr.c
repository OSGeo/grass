#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <tcl.h>
#include <tk.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/form.h>
#include "global.h"
#include "proto.h"

static int last_cat_line;

int del_cat (int line, int field, int cat ) 
{
    int type, i;
    static struct line_pnts *Points = NULL;
    static struct line_cats *Cats = NULL;
    char buf[1000];
    
    G_debug (3, "del_cat() line = %d, field = %d, cat = %d", line, field, cat);

    if ( Points == NULL ) Points = Vect_new_line_struct ();
    if ( Cats == NULL ) Cats = Vect_new_cats_struct ();

    type = Vect_read_line(&Map, Points, Cats, line);
    Vect_field_cat_del ( Cats, field, cat);

    last_cat_line = Vect_rewrite_line (&Map, line, type, Points, Cats);
	
    check_record ( field, cat );

    Tcl_Eval ( Toolbox, "clear_cats" );	
    
    for (i = 0; i < Cats->n_cats; i++) {
	sprintf ( buf, "add_cat %d %d %d", last_cat_line, Cats->field[i], Cats->cat[i]);
	Tcl_Eval ( Toolbox, buf );
    }
    
    symb_updated_lines_set_from_map();
    symb_updated_nodes_set_from_map();
    G_debug (2, "  last_cat_line = %d", last_cat_line);
    
    return 0;
}

int add_cat (int field, int cat, int newrec ) 
{
    int type, i, ret;
    static struct line_pnts *Points = NULL;
    static struct line_cats *Cats = NULL;
    char buf[1000];
    
    G_debug (2, "add_cat() last_cat_line = %d, field = %d, cat = %d, newrec = %d", 
	                last_cat_line, field, cat, newrec);

    if ( Points == NULL ) Points = Vect_new_line_struct ();
    if ( Cats == NULL ) Cats = Vect_new_cats_struct ();

    type = Vect_read_line(&Map, Points, Cats, last_cat_line);
    Vect_cat_set ( Cats, field, cat );

    last_cat_line = Vect_rewrite_line (&Map, last_cat_line, type, Points, Cats);

    Tcl_Eval ( Toolbox, "clear_cats" );	
    
    for (i = 0; i < Cats->n_cats; i++) {
	sprintf ( buf, "add_cat %d %d %d", last_cat_line, Cats->field[i], Cats->cat[i]);
	Tcl_Eval ( Toolbox, buf );
    }

    if ( newrec ) {
	ret = new_record ( field, cat );
        if ( ret == 0 ) {
	    G_debug (2, "New record created.");
	} else if ( ret == 1 ) {
	    G_debug (2, "Record already existed.");
	} else if ( ret == -1 ) {
	    G_warning ("Cannot create new record.");
	}
    }
    
    symb_updated_lines_set_from_map();
    symb_updated_nodes_set_from_map();
    G_debug (2, "  last_cat_line = %d", last_cat_line);
    
    return 0;
}

/* 
 * Create new record in table 
 * returns: 0 created
 *          1 existed
 *         -1 error  
 */
int new_record ( int field, int cat ) 
{
    int ret, old;
    struct field_info *Fi;
    dbDriver *driver;
    dbValue value;
    dbString sql;
    char buf[1000];
    
    db_init_string (&sql);

    G_debug (2, "new_record() field = %d cat = %d", field, cat );
    
    Fi = Vect_get_field( &Map, field );
    if ( Fi == NULL ) { 
	i_message ( MSG_OK, MSGI_ERROR, "Database table for this layer is not defined" );
	return -1;
    }

    /* Note: some drivers (dbf) writes date when db is closed so it is better open
     * and close database for each record, so that data may not be lost later */

    /* First check if already exists */
    driver = db_start_driver_open_database ( Fi->driver, Fi->database );
    if ( driver == NULL ) {
	sprintf (buf, "Cannot open database %s by driver %s", Fi->database, Fi->driver );
	i_message ( MSG_OK, MSGI_ERROR, buf );
	return -1;
    }
    ret = db_select_value ( driver, Fi->table, Fi->key, cat, Fi->key, &value );
    if ( ret == -1 ) {
	db_close_database_shutdown_driver ( driver );
	sprintf (buf, "Cannot select record from table %s", Fi->table );
	i_message ( MSG_OK, MSGI_ERROR, buf );
	return -1;
    }
    if ( ret == 0 ) { /* insert new record */
	sprintf ( buf, "insert into %s (%s) values (%d)", Fi->table, Fi->key, cat );
	db_set_string ( &sql, buf);
	G_debug ( 2, db_get_string ( &sql ) );
	ret = db_execute_immediate (driver, &sql);
	if ( ret != DB_OK ) {	
	    db_close_database_shutdown_driver ( driver );
	    sprintf (buf, "Cannot insert new record: %s", db_get_string(&sql) );
	    i_message ( MSG_OK, MSGI_ERROR, buf );
	    return -1;
	}
	old = 0;
    } else { /* record already existed */
	old = 1;
    }
    
    db_close_database_shutdown_driver ( driver );

    return old;
}


/* Display categories */

struct display_cats
{
    double thresh;
    struct line_pnts *Points;
    struct line_cats *Cats;
};

int display_cats_begin(void *closure)
{
    struct display_cats *dc = closure;

    G_debug (2, "display_cats()");

    dc->Points = Vect_new_line_struct ();
    dc->Cats = Vect_new_cats_struct ();
    
    i_prompt ( "Display categories:"); 
    i_prompt_buttons ( "Select line", "", "Quit tool"); 
    
    /* TODO: use some better threshold */
    dc->thresh = fabs ( D_d_to_u_col ( 10 ) - D_d_to_u_col ( 0 ) ) ; 
    G_debug (2, "thresh = %f", dc->thresh );
    
    F_clear ();
    last_cat_line = 0;

    set_mode(MOUSE_POINT);

    return 0;
}

int display_cats_update(void *closure, int sxn, int syn, int button)
{
    struct display_cats *dc = closure;
    double x =  D_d_to_u_col ( sxn );
    double y =  D_d_to_u_row ( syn );

    G_debug (2, "button = %d x = %d = %f y = %d = %f", button, sxn, x, syn, y);

    /* Display last highlighted in normal color */
    G_debug (2, "  last_cat_line = %d", last_cat_line);
    if ( last_cat_line > 0 )
	display_line ( last_cat_line, SYMB_DEFAULT, 1);

    if (button == 3) /* Quit tool */
	return 1;

    if (button == 1) /* Confirm / select */
    {
	int j, line;
	F_clear ();
	/* Find nearest point or line (points first!) */
	line = Vect_find_line (&Map, x, y, 0, GV_POINTS, dc->thresh, 0, 0);
	G_debug (2, "point found = %d", line );
	if ( line == 0 )
	    line = Vect_find_line (&Map, x, y, 0, GV_LINE|GV_BOUNDARY, dc->thresh, 0, 0);
	G_debug (2, "line found = %d", line );
	    
	/* Display new selected line if any */
	if ( line > 0 )
	{
	    int type;
	    display_line ( line, SYMB_HIGHLIGHT, 1);
	    type = Vect_read_line(&Map, dc->Points, dc->Cats, line);

	    Tcl_Eval ( Toolbox, "mk_cats" ); /* mk_cats checks if already opened */
	    Tcl_Eval ( Toolbox, "clear_cats" );	
		    
	    for (j = 0; j < dc->Cats->n_cats; j++)
	    {
		char buf[1000];
		G_debug(3, "field = %d category = %d", dc->Cats->field[j], dc->Cats->cat[j]);

		sprintf ( buf, "add_cat %d %d %d", line, dc->Cats->field[j], dc->Cats->cat[j]);
		Tcl_Eval ( Toolbox, buf );

	    }
	}

	last_cat_line = line;
    }

    return 0;
}

int display_cats_end(void *closure)
{
    Tcl_Eval ( Toolbox, "destroy_cats" );
    
    i_prompt (""); 
    i_prompt_buttons ( "", "", ""); 
    i_coor ( COOR_NULL, COOR_NULL); 
    
    G_debug (3, "display_cats(): End");
    return 1;
}

void display_cats(void)
{
    static struct display_cats dc;

    set_tool(display_cats_begin, display_cats_update, display_cats_end, &dc);
}

/* Copy categories from one feature to another */
struct copy_cats
{
    int src_line, dest_line;
    double thresh;
    struct line_pnts *Points;
    struct line_cats *Src_Cats, *Dest_Cats;
};

int copy_cats_begin(void *closure)
{
    struct copy_cats *cc = closure;
    
    G_debug (2, "copy_cats()");

    cc->Points = Vect_new_line_struct ();
    cc->Src_Cats = Vect_new_cats_struct ();
    cc->Dest_Cats = Vect_new_cats_struct ();
    
    i_prompt ( "Copy attributes:"); 
    i_prompt_buttons ( "Select source object", "", "Quit tool"); 
    
    /* TODO: use some better threshold */
    cc->thresh = fabs ( D_d_to_u_col ( 10 ) - D_d_to_u_col ( 0 ) ) ; 
    G_debug (2, "thresh = %f", cc->thresh );
    
    cc->src_line = 0;
    cc->dest_line = 0;

    set_mode(MOUSE_POINT);

    return 0;
}

int copy_cats_update(void *closure, int sxn, int syn, int button)
{
    struct copy_cats *cc = closure;
    double x =  D_d_to_u_col ( sxn );
    double y =  D_d_to_u_row ( syn );
    G_debug (3, "button = %d x = %d = %f y = %d = %f", button, sxn, x, syn, y);

    if (button==3) /* Quit tool */
	return 1;
        
    if (cc->src_line>0)
	display_line (cc->src_line, SYMB_DEFAULT, 1);
    if (cc->dest_line>0)
	display_line (cc->dest_line, SYMB_DEFAULT, 1);
    if (button==1) {
	int line = Vect_find_line (&Map, x, y, 0, GV_LINES|GV_POINTS, cc->thresh, 0, 0);
	G_debug (3, "before: src_line=%d dest_line=%d line=%d",cc->src_line,cc->dest_line,line);
	if (cc->dest_line>0) {
	    int new_line, type, i;
	    /* We have a source- and a destination-object
	     * => copy categories */
	    type = Vect_read_line (&Map, cc->Points, cc->Dest_Cats, cc->dest_line);
	    new_line = Vect_rewrite_line (&Map, cc->dest_line, type, cc->Points, cc->Src_Cats);
	    if (line==cc->dest_line)
		line = new_line;
	    cc->dest_line = new_line;
                
	    for (i=0; i<cc->Dest_Cats->n_cats; i++) {
		check_record (cc->Dest_Cats->field[i], cc->Dest_Cats->cat[i]);
	    }
                
	    updated_lines_and_nodes_erase_refresh_display ();
                
	    /* move the selections on */
	    cc->src_line = cc->dest_line;
	    cc->dest_line = line;
	} else if (cc->src_line>0) {
	    /* We have a source-object and possibly a destination object
	     * was selected */
	    if (line<=0)
		cc->src_line = 0;
	    else if (line!=cc->src_line)
		cc->dest_line = line;
	} else {
	    /* We have no object selected and possible a source-object
	     * was selected => read its categories into Src_Cats */
	    cc->src_line = line;
	    if (cc->src_line>0)
		Vect_read_line (&Map, cc->Points, cc->Src_Cats, cc->src_line);
	}
	G_debug (3, "after: src_line=%d dest_line=%d line=%d",cc->src_line,cc->dest_line,line);
    } else if (button==2) {
	/* We need to deselect the last line selected */
	if (cc->dest_line>0) {
	    display_line (cc->dest_line, SYMB_DEFAULT, 1);
	    cc->dest_line = 0;
	} else if (cc->src_line>0) {
	    display_line (cc->src_line, SYMB_DEFAULT, 1);
	    cc->src_line = 0;
	}
    }
                
    /* Display the selected lines accordingly and set the button prompts */
    if (cc->dest_line>0) {
	display_line (cc->dest_line, SYMB_HIGHLIGHT, 1);
	display_line (cc->src_line, SYMB_HIGHLIGHT, 1);
	i_prompt("Select the target object");
	i_prompt_buttons("Conform and select next","Deselect Target","Quit tool");
    } else if (cc->src_line>0) {
	display_line (cc->src_line, SYMB_HIGHLIGHT, 1);
	i_prompt("Select the target object");
	i_prompt_buttons("Select","Deselect Source","Quit tool");
    } else {
	i_prompt ( "Copy attributes:"); 
	i_prompt_buttons ( "Select source object", "", "Quit tool"); 
    }

    return 0;
}

int copy_cats_end(void *closure)
{
    struct copy_cats *cc = closure;

    if (cc->dest_line>0)
        display_line (cc->dest_line, SYMB_DEFAULT, 1);
    if (cc->src_line>0)
        display_line (cc->src_line, SYMB_DEFAULT, 1);
    
    i_prompt (""); 
    i_prompt_buttons ( "", "", ""); 
    i_coor ( COOR_NULL, COOR_NULL);
    
    Vect_destroy_line_struct (cc->Points);
    Vect_destroy_cats_struct (cc->Src_Cats);
    Vect_destroy_cats_struct (cc->Dest_Cats);
    
    G_debug (3, "copy_cats(): End");
    
    return 1;
}

void copy_cats(void)
{
    static struct copy_cats cc;

    set_tool(copy_cats_begin, copy_cats_update, copy_cats_end, &cc);
}

/* Display attributes */

struct display_attributes
{
    double thresh;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int last_line;
    dbString html; 
};

int display_attributes_begin(void *closure)
{
    struct display_attributes *da = closure;
    
    G_debug (2, "display_attributes()");

    da->Points = Vect_new_line_struct ();
    da->Cats = Vect_new_cats_struct ();
    
    i_prompt ( "Display attributes:"); 
    i_prompt_buttons ( "Select line", "", "Quit tool"); 
    
    /* TODO: use some better threshold */
    da->thresh = fabs ( D_d_to_u_col ( 10 ) - D_d_to_u_col ( 0 ) ) ; 
    G_debug (2, "thresh = %f", da->thresh );
    
    F_clear ();
    da->last_line = 0;

    db_init_string(&da->html);

    set_mode(MOUSE_POINT);

    return 0;
}

int display_attributes_update(void *closure, int sxn, int syn, int button)
{
    static int first_form = 1;
    struct display_attributes *da = closure;
    double x =  D_d_to_u_col ( sxn );
    double y =  D_d_to_u_row ( syn );
    G_debug (3, "button = %d x = %d = %f y = %d = %f", button, sxn, x, syn, y);

    /* Display last highlighted in normal color */
    if ( da->last_line > 0 ) {
	display_line ( da->last_line, SYMB_DEFAULT, 1);
    }

    if (button == 3) /* Quit tool */
	return 1;

    if ( button == 1 ) { /* Confirm / select */
	int line;
	F_clear ();
	/* Find nearest point or line (points first!) */
	line = Vect_find_line (&Map, x, y, 0, GV_POINTS, da->thresh, 0, 0);
	G_debug (2, "point found = %d", line );
	if ( line == 0 ) line = Vect_find_line (&Map, x, y, 0, GV_LINE|GV_BOUNDARY, da->thresh, 0, 0);
	G_debug (2, "line found = %d", line );
	    
	/* Display new selected line if any */
	if ( line > 0 ) {
	    char buf[1000], title[500];
	    int type;
	    display_line ( line, SYMB_HIGHLIGHT, 1);
	    type = Vect_read_line(&Map, da->Points, da->Cats, line);

	    /* Note: F_open() must be run first time with closed monitor, otherwise next
	     *         *        attempt to open driver hangs until form child process is killed */
	    if ( first_form ) { 
		driver_close();
		F_open ( "", "" );
		F_clear ();
		driver_open(); 
		first_form = 0; 
	    }

	    if ( da->Cats->n_cats > 0 ) {
		int j;
		for (j = 0; j < da->Cats->n_cats; j++) {
		    struct field_info *Fi;
		    char *form;

		    G_debug(3, "field = %d category = %d", da->Cats->field[j], da->Cats->cat[j]);

		    sprintf (title, "Layer %d", da->Cats->field[j] );
		    db_set_string (&da->html, ""); 
		    db_append_string (&da->html, "<HTML><HEAD><TITLE>Attributes</TITLE><BODY>"); 

		    sprintf(buf, "layer: %d<BR>category: %d<BR>", da->Cats->field[j], da->Cats->cat[j] );
		    db_append_string (&da->html, buf);

		    Fi = Vect_get_field( &Map, da->Cats->field[j]);
		    if (Fi == NULL) {
			db_append_string (&da->html, "Database connection not defined<BR>" );
		    } else {
			sprintf(buf, "driver: %s<BR>database: %s<BR>table: %s<BR>key column: %s<BR>",
				Fi->driver, Fi->database, Fi->table, Fi->key);
			db_append_string (&da->html, buf);
			    
			F_generate ( Fi->driver, Fi->database, Fi->table, Fi->key, da->Cats->cat[j], 
				     NULL, NULL, F_EDIT, F_HTML, &form);
			    
			db_append_string (&da->html, form); 
			G_free (form);
			G_free(Fi);
		    }
		    db_append_string (&da->html, "</BODY></HTML>"); 
		    G_debug ( 3, db_get_string (&da->html) ); 
		    F_open ( title, db_get_string(&da->html) );
		}
	    } else {
		sprintf (title, "Line %d", line );
		db_init_string(&da->html);
		db_set_string (&da->html, ""); 
		db_append_string (&da->html, "<HTML><HEAD><TITLE>Attributes</TITLE><BODY>"); 
		db_append_string (&da->html, "No categories"); 
		db_append_string (&da->html, "</BODY></HTML>"); 
		G_debug ( 3, db_get_string (&da->html) ); 
		F_open ( title, db_get_string(&da->html) );
	    }
	}
	da->last_line = line;
    }

    return 0;
}

int display_attributes_end(void *closure)
{
    F_clear ();
    F_close ();
    
    i_prompt (""); 
    i_prompt_buttons ( "", "", ""); 
    i_coor ( COOR_NULL, COOR_NULL); 
    
    G_debug (3, "display_attributes(): End");

    return 1;
}

void display_attributes(void)
{
    static struct display_attributes da;

    set_tool(display_attributes_begin, display_attributes_update, display_attributes_end, &da);
}

/* 
 * Check if deleted category exists in category index, ask user if not and delete it if requested
 * 
 * returns: 
 */
int check_record ( int field, int cat ) 
{
    int ret, field_index, type, id;
    struct field_info *Fi;
    dbDriver *driver;
    dbValue value;
    dbString sql;
    char buf[1000];
    
    db_init_string (&sql);

    G_debug (3, "check_record() field = %d cat = %d", field, cat );

    Fi = Vect_get_field( &Map, field );
    if ( Fi == NULL ) {  /* no table */
	return 0;
    }

    /* Are there still elemets with this category */
    field_index = Vect_cidx_get_field_index ( &Map, field );
    G_debug (3, "field_index = %d", field_index );
    if ( field_index >= 0 ) {
        ret = Vect_cidx_find_next ( &Map, field_index, cat, GV_POINTS|GV_LINES, 0, &type, &id );
	G_debug (3, "ret = %d", ret );

	if ( ret >= 0 ) return 0; /* Category exists in map */
    }

    /* Does record exist ? */
    driver = db_start_driver_open_database ( Fi->driver, Fi->database );
    if ( driver == NULL ) {
	sprintf (buf, "Cannot open database %s by driver %s", Fi->database, Fi->driver );
	i_message ( MSG_OK, MSGI_ERROR, buf );
	return -1;
    }
    ret = db_select_value ( driver, Fi->table, Fi->key, cat, Fi->key, &value );
    G_debug (3, "n records = %d", ret );
    if ( ret == -1 ) {
	db_close_database_shutdown_driver ( driver );
	sprintf (buf, "Cannot select record from table %s", Fi->table );
	i_message ( MSG_OK, MSGI_ERROR, buf );
	return -1;
    }
    
    if ( ret == 0 ) return 0;

    sprintf (buf, "There are no more features with category %d (layer %d) in the map, but there is "
	          "record in the table. Delete this record?", cat, field );
    ret = i_message ( MSG_YESNO, MSGI_QUESTION, buf );
    
    if ( ret == 1 ) return 0;  /* No, do not delete */

    sprintf ( buf, "delete from %s where %s = %d", Fi->table, Fi->key, cat );
    db_set_string ( &sql, buf);
    G_debug ( 2, db_get_string ( &sql ) );
    ret = db_execute_immediate (driver, &sql);
    if ( ret != DB_OK ) {	
	db_close_database_shutdown_driver ( driver );
	sprintf (buf, "Cannot delete record: %s", db_get_string(&sql) );
	i_message ( MSG_OK, MSGI_ERROR, buf );
	return -1;
    }
    
    db_close_database_shutdown_driver ( driver );

    return 0;
}

