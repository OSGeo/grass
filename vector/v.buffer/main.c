/****************************************************************
 *
 * MODULE:       v.buffer
 * 
 * AUTHOR(S):    Radim Blazek
 *               
 * PURPOSE:      Vector buffer
 *               
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/
#include <stdlib.h> 
#include <string.h> 
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#define DEBUG_NONE   0
#define DEBUG_BUFFER 1
#define DEBUG_CLEAN  2

#define PI M_PI

/* TODO: look at RET value and use, is it OK? */
#define RET 0.000000001 /* Representation error tolerance */

/* returns shortest distance to nearest input feature within the buffer 
 * or a number greater than buffer (2*buffer) if not found */
double input_distance ( struct Map_info *In, int type, double buffer, double x, double y ) 
{
    int i;
    static struct ilist *List = NULL;
    static struct line_pnts *Points = NULL;
    double current_distance;

    BOUND_BOX box;

    if ( List == NULL ) 
	List = Vect_new_list ();
    else
	Vect_reset_list ( List );

    if ( Points == NULL ) 
	Points = Vect_new_line_struct ();


    /* Inside area ? */
    if ( type & GV_AREA) {
	int area, centroid;
	/* inside */
	area = Vect_find_area ( In, x, y );
	centroid = 0;
	if ( area )
	    centroid = Vect_get_area_centroid ( In, area );

	G_debug ( 3, "    area = %d centroid = %d", area, centroid );
	if ( centroid ) 
	    return 0.0;
    }

    /* ouside area, within buffer? */
    /* The centroid is in buffer if at least one point/line/boundary is in buffer distance */
    box.E = x + buffer; box.W = x - buffer;
    box.N = y + buffer; box.S = y - buffer;
    box.T = PORT_DOUBLE_MAX; box.B = -PORT_DOUBLE_MAX;
    
    Vect_select_lines_by_box ( In, &box, GV_POINTS | GV_LINES, List );
    G_debug ( 3, "  %d lines selected by box", List->n_values );

    current_distance = 2*buffer;
    for ( i = 0; i < List->n_values; i++) {
	int line, ltype;
	double dist;
	line = List->value[i];
    
	G_debug ( 3, "    line[%d] = %d", i, line );

	ltype = Vect_read_line (In, Points, NULL, line);
	
	Vect_line_distance ( Points, x, y, 0., 0, NULL, NULL, NULL, &dist, NULL, NULL);
	G_debug ( 3, "    dist = %f", dist );
	if ( dist > buffer ) continue;
	
	/* lines */
	if ( type & ltype ) {
	    if ( dist < current_distance )
	        current_distance = dist;
	}

	/* Areas */ 
	if ( (type & GV_AREA) && ltype == GV_BOUNDARY ) {
	    int j, side[2], centr[2], area_in;
	    
	    Vect_get_line_areas ( In, line, &side[0], &side[1] );

	    for ( j = 0; j < 2; j++ ) { 
		centr[j] = 0;

		if ( side[j] > 0 )
		    area_in = side[j];
		else /* island */
		    area_in = Vect_get_isle_area ( In, abs ( side[j] ) );

		if ( area_in > 0 )
		    centr[j] = Vect_get_area_centroid ( In, area_in );
	    }
	
	    if ( centr[0] || centr[1] ) {
		if ( dist < current_distance )
		    current_distance = dist;
	    }
	}	    
    }
    return current_distance;
}

/* 
 * Test if area in Out is in buffer.
 * Return: 1 in buffer
 *         0 outside buffer
 */
int area_in_buffer ( struct Map_info *In, struct Map_info *Out, 
	             int area, int type, double buffer, double tolerance )
{
    double dist;
    int    ret, i;
    static struct ilist *List = NULL;
    static struct line_pnts *Points = NULL;
    double x, y;

    G_debug ( 3, "  area_in_buffer(): area = %d", area );

    if ( List == NULL ) 
	List = Vect_new_list ();

    if ( Points == NULL )
	Points = Vect_new_line_struct ();

    /* Warning: because of tolerance, centroid in area calculated by Vect_get_point_in_area()
     * may fall into buffer (arc interpolated by polygon) even if area is outside!!! */

    /* Because of finite number of decimal places in double representation, RET is used. */

    /* Test1: Centroid (the only reliable, I think).
     * There are 3 possible cases for the distance of centroid to nearest input feature:
     * 1) < (buffer - tolerance) => area inside the buffer 
     * 2) > buffer => area outside the buffer
     * 3) > (buffer - tolerance) and < buffer (that means within the tolerance) => uncertain */
    ret = Vect_get_point_in_area ( Out, area, &x, &y );
    if ( ret == 0 ) { /* centroid OK */
	/* test the distance to nearest input feature */
	dist = input_distance ( In, type, buffer, x, y); 
	G_debug ( 3, "  centroid %f, %f  dist = %f", x, y, dist );

	if ( dist < (buffer - tolerance - RET) ) { 
	    G_debug ( 3, "    centroid in buffer -> area in buffer");
	    return 1;
	} else if ( dist > (buffer + RET) ) {
	    G_debug ( 3, "    centroid outside buffer -> area outside buffer");
	    return 0;
	}
    }

    /* Test2: counterclockwise (ccw) boundary
     * Boundaries around features are written in ccw order, that means area inside buffer is on the
     * left side. Area bundaries are listed in cw order (ccw boundaries as negative). 
     * So if any boundary in area list is negative, i.e. in cww order, i.e. buffer inside area,
     * whole area _must_ be in buffer. But, also areas without such boundary may be in buffer, 
     * see below. */ 
    /* TODO: The problem!!!: unfortunately it is not true. After snap, break and remove duplicate
     * it may happen that ccw line appeareas in the area outside the buffer!!! */
    Vect_get_area_boundaries ( Out, area, List ); 

    for ( i = 0; i <  List->n_values; i++ ) {
	if ( List->value[i] < 0 ) { 
	    G_debug ( 3, "    negative boundary -> area in buffer");
	    return 1;
	}
    }

    /* Test3: Input feature within area.
     * I believe, that if no negative boundary was found and area is in buffer, 
     * the distance of the nearest input feature for at least one area vertex must be < (buffer-tolerance) 
     * This test is left as last because it is slow (check each vertex). */
    Vect_get_area_points ( Out, area, Points );
    for ( i = 0; i < Points->n_points - 1; i++ ) {
	dist = input_distance ( In, type, buffer, Points->x[i], Points->y[i]); 
	G_debug ( 3, "  vertex dist = %f", dist );

	if ( dist < (buffer - tolerance - RET) ) {
	    G_debug ( 3, "    vertex in buffer -> area in buffer");
	    return 1;
	}
    }

    /* Test4?: It may be that that Test3 does not cover all possible cases. If so, somebody must
     * find such example and send it to me */

    G_debug ( 3, "    -> area outside buffer");
    return 0;
}

void stop (  struct Map_info *In,  struct Map_info *Out ) 
{
    Vect_close (In);

    G_message ( _("Rebuilding topology...") );
    Vect_build_partial ( Out, GV_BUILD_NONE, NULL );
    Vect_build (Out, stderr);
    Vect_close (Out);
}

int 
main (int argc, char *argv[])
{
    struct Map_info In, Out;
    struct line_pnts *Points, *BPoints;
    struct line_cats *Cats, *BCats;
    char   *mapset;
    struct GModule *module;
    struct Option *in_opt, *out_opt, *type_opt, *buffer_opt, *tolerance_opt, 
		*bufcol_opt, *scale_opt, *debug_opt, *field_opt;
    double buffer, tolerance, dtmp;
    int    type, debug;
    int    ret, nareas, area, nlines, line;
    char   *Areas, *Lines;
    int    field;

 /* Attributes if sizecol is used */
    int i, nrec, ctype;
    struct field_info *Fi;
    dbDriver *Driver;
    dbCatValArray cvarr;
    int size_val_int;
    double size_val, scale, orig_tolerance;


    module = G_define_module();
    module->keywords = _("vector");
    module->description =
	_("Creates a buffer around features of given type (areas must contain centroid).");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    
    type_opt = G_define_standard_option(G_OPT_V_TYPE) ;
    type_opt->options = "point,line,boundary,centroid,area";
    type_opt->answer = "point,line,area";

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    buffer_opt = G_define_option();
    buffer_opt->key = "buffer";
    buffer_opt->type = TYPE_DOUBLE;
    buffer_opt->required = NO;
    buffer_opt->description = _("Buffer distance in map units");

    bufcol_opt = G_define_option();
    bufcol_opt->key = "bufcol";
    bufcol_opt->type = TYPE_STRING;
    bufcol_opt->required = NO;
    bufcol_opt->description = _("Attribute column to use for buffer distances");
    bufcol_opt->guisection  = _("Advanced");

    scale_opt = G_define_option();
    scale_opt->key = "scale";
    scale_opt->type = TYPE_DOUBLE;
    scale_opt->required = NO;
    scale_opt->answer = "1.0";
    scale_opt->description = _("Scaling factor for attribute column values");
    scale_opt->guisection  = _("Advanced");

    tolerance_opt = G_define_option();
    tolerance_opt->key = "tolerance";
    tolerance_opt->type = TYPE_DOUBLE;
    tolerance_opt->required = NO;
    tolerance_opt->answer = "0.01";
    tolerance_opt->guisection  = _("Advanced");
    tolerance_opt->description =
	_("Maximum distance between theoretical arc and polygon segments "
		"as multiple of buffer");

    debug_opt = G_define_option();
    debug_opt->key = "debug";
    debug_opt->type = TYPE_STRING;
    debug_opt->required = NO;
    debug_opt->options = "buffer,clean";
    debug_opt->guisection  = _("Advanced");
    debug_opt->description = _("Stop the process at a certain stage");

    G_gisinit(argv[0]);
    if (G_parser (argc, argv))
	exit(EXIT_FAILURE); 
    
    type = Vect_option_to_types ( type_opt );
    field = atoi( field_opt->answer );

    if ( (buffer_opt->answer && bufcol_opt->answer) ||
	  (! (buffer_opt->answer || bufcol_opt->answer)) )
	G_fatal_error("Select a buffer distance or column, but not both.");

    if(bufcol_opt->answer)
	G_warning(_("The bufcol option may contain bugs during the cleaning "
		"step. If you encounter problems, use the debug "
		"option or clean manually with v.clean tool=break; "
		"v.category step=0; v.extract -d type=area"));

    orig_tolerance = atof( tolerance_opt->answer );
    tolerance = orig_tolerance;

    scale = atof( scale_opt->answer );
    if ( scale <= 0.0 )
	G_fatal_error("Illegal scale value");

    if(buffer_opt->answer) {
	buffer = fabs ( atof( buffer_opt->answer ) );

	tolerance *= buffer;
	G_message(_("The tolerance in map units: %g"), tolerance );

	/* At least 8 points for circle. */
	dtmp = 0.999 * buffer * ( 1 - cos ( 2 * PI / 8 / 2 ) );
	G_debug ( 3, "Minimum tolerance = %f", dtmp );
	if ( tolerance > dtmp ) {
	    tolerance = dtmp;
	    G_warning(_("The tolerance was reset to %g (map units)"), tolerance );
	}
    }

    debug = DEBUG_NONE;
    if ( debug_opt->answer ) {
	if ( debug_opt->answer[0] == 'b' ) 
	   debug = DEBUG_BUFFER; 
	else if ( debug_opt->answer[0] == 'c' ) 
	   debug = DEBUG_CLEAN; 
    }

    Vect_check_input_output_name ( in_opt->answer, out_opt->answer, GV_FATAL_EXIT );

    Points = Vect_new_line_struct ();
    BPoints = Vect_new_line_struct ();
    Cats = Vect_new_cats_struct ();
    BCats = Vect_new_cats_struct ();

    /* open input vector */
    if ((mapset = G_find_vector2 (in_opt->answer, "")) == NULL) {
	 G_fatal_error (_("Vector map <%s> not found"), in_opt->answer);
    }

    Vect_set_open_level (2); 
    Vect_open_old (&In, in_opt->answer, mapset); 

    Vect_set_fatal_error (GV_FATAL_PRINT);
    if (0 > Vect_open_new (&Out, out_opt->answer, 0) ) {
	 Vect_close (&In);
	 exit (EXIT_FAILURE);
    }

    /* check and load attribute column data */
    if(bufcol_opt->answer) {
	db_CatValArray_init ( &cvarr );

	Fi = Vect_get_field( &In, field );
	if ( Fi == NULL )
	    G_fatal_error(_("Unable to get layer info for vector map"));

	Driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (Driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"), Fi->database, Fi->driver);

	/* Note do not check if the column exists in the table because it may be expression */

/* TODO: only select values we need instead of all in column */
	nrec = db_select_CatValArray(Driver, Fi->table, Fi->key,
		bufcol_opt->answer, NULL, &cvarr );

	if ( nrec < 0 ) G_fatal_error (_("Unable to select data from table"));
	G_debug(2, "%d records selected from table", nrec);

	ctype = cvarr.ctype;
	if ( ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE )
	    G_fatal_error (_("Column type not supported"));

	db_close_database_shutdown_driver(Driver);

	for ( i = 0; i < cvarr.n_values; i++ ) {
	    if ( ctype == DB_C_TYPE_INT ) {
		G_debug (4, "cat = %d val = %d", cvarr.value[i].cat, cvarr.value[i].val.i );
	    } else if ( ctype == DB_C_TYPE_DOUBLE ) {
		G_debug (4, "cat = %d val = %f", cvarr.value[i].cat, cvarr.value[i].val.d );
	    }
	}

    }


    Vect_copy_head_data (&In, &Out);
    Vect_hist_copy (&In, &Out);
    Vect_hist_command ( &Out );

    /* Create buffers' boundaries */

    /* Lines (and Points) */
    if ( (type & GV_POINTS) || (type & GV_LINES) ) {
	int nlines, line, ltype;

	nlines = Vect_get_num_lines ( &In );
	
        G_message ( _("Lines buffers... "));
	for ( line = 1; line <= nlines; line++ ) {
	    int cat;
	    
	    G_debug ( 3, "line = %d", line );
	    G_percent ( line, nlines, 2 );

	    ltype = Vect_read_line (&In, Points, Cats, line);
	    if ( !(ltype & type ) ) continue;

	    if ( !Vect_cat_get( Cats, field, &cat ) ) continue;

	    if(bufcol_opt->answer) {
		/* get value from sizecol column */
/* should probably be put in a function */
		if( ctype == DB_C_TYPE_INT ) {
		    ret = db_CatValArray_get_value_int(&cvarr, cat, &size_val_int);
		    if ( ret != DB_OK ) {
			G_warning(_("No record for category %d in table <%s>"), cat, Fi->table );
			continue;
		    }
		    size_val = (double)size_val_int;
		}

		if( ctype == DB_C_TYPE_DOUBLE ) {
		    ret = db_CatValArray_get_value_double(&cvarr, cat, &size_val);
		    if ( ret != DB_OK ) {
			G_warning(_("No record for category %d in table <%s>"), cat, Fi->table );
			continue;
		    }
		}

		if (size_val < 0.0) {
		    G_warning(_("Attribute is of invalid size (%.3f) for category %d"), 
					size_val, cat);
		    continue;
		}

		if (size_val == 0.0) continue;

		buffer = size_val * scale;
		G_debug(2, "    dynamic buffer size = %.2f", buffer);

		tolerance = orig_tolerance * buffer;
		G_debug(2, _("The tolerance in map units: %g"), tolerance );

		/* At least 8 points for circle. */
		dtmp = 0.999 * buffer * ( 1 - cos ( 2 * PI / 8 / 2 ) );
		G_debug(2, "Minimum tolerance = %f", dtmp );
		if ( tolerance > dtmp ) {
		    tolerance = dtmp;
		    G_warning(_("The tolerance was reset to %g (map units). [category %d]"),
				tolerance, cat);
		}
	    }


	    Vect_line_buffer ( Points, buffer, tolerance, BPoints );	
	    Vect_write_line ( &Out, GV_BOUNDARY, BPoints, BCats );  
	}
    }

    /* Areas */
    if ( type & GV_AREA ) {
	int i, nareas, area, centroid, nisles, isle;
	
	nareas = Vect_get_num_areas ( &In );

        G_message ( _("Areas buffers... "));
	for ( area = 1; area <= nareas; area++ ) {
	    int cat;

	    G_percent ( area, nareas, 2 );
	    
	    centroid = Vect_get_area_centroid ( &In, area );
	    if ( centroid == 0 ) continue;
	    
	    Vect_read_line (&In, NULL, Cats, centroid);
	    if ( !Vect_cat_get( Cats, field, &cat ) ) continue;

	    if(bufcol_opt->answer) {
		/* get value from sizecol column */

		if( ctype == DB_C_TYPE_INT ) {
		    ret = db_CatValArray_get_value_int(&cvarr, cat, &size_val_int);
		    if ( ret != DB_OK ) {
			G_warning(_("No record for category %d in table <%s>"), cat, Fi->table );
			continue;
		    }
		    size_val = (double)size_val_int;
		}

		if( ctype == DB_C_TYPE_DOUBLE ) {
		    ret = db_CatValArray_get_value_double(&cvarr, cat, &size_val);
		    if ( ret != DB_OK ) {
			G_warning(_("No record for category %d in table <%s>"), cat, Fi->table );
			continue;
		    }
		}

		if (size_val < 0.0) {
		    G_warning(_("Attribute is of invalid size (%.3f) for category %d"), 
					size_val, cat);
		    continue;
		}

		if (size_val == 0.0) continue;

		buffer = size_val * scale;
		G_debug(2, "    dynamic buffer size = %.2f", buffer);

		tolerance = orig_tolerance * buffer;
		G_debug(2, _("The tolerance in map units: %g"), tolerance );

		/* At least 8 points for circle. */
		dtmp = 0.999 * buffer * ( 1 - cos ( 2 * PI / 8 / 2 ) );
		G_debug(2, "Minimum tolerance = %f", dtmp );
		if ( tolerance > dtmp ) {
		    tolerance = dtmp;
		    G_warning(_("The tolerance was reset to %g (map units). [category %d]"),
				tolerance, cat);
		}
	    }


	    /* outer ring */
	    Vect_get_area_points ( &In, area, Points );
	    Vect_line_buffer ( Points, buffer, tolerance, BPoints );	
	    Vect_write_line ( &Out, GV_BOUNDARY, BPoints, BCats );  
	    
	    /* islands */
	    nisles = Vect_get_area_num_isles (&In, area);
	    for ( i = 0; i < nisles; i++ ) {
		double l;
		
		isle = Vect_get_area_isle (&In, area, i);
		Vect_get_isle_points ( &In, isle, Points );

		/* Check if the isle is big enough */
		l = Vect_line_length ( Points );
		if ( l/2 < 2*buffer ) continue;
		
		Vect_line_buffer ( Points, buffer, tolerance, BPoints );	
		Vect_write_line ( &Out, GV_BOUNDARY, BPoints, BCats );  
	    }
	}
    }

    if ( debug == DEBUG_BUFFER ) {
        stop ( &In, &Out );
	exit (EXIT_SUCCESS);
    }

    /* Create areas */
    
    /* Break lines */
    G_message (_("Building parts of topology...") );
    Vect_build_partial ( &Out, GV_BUILD_BASE, stderr );

    /* Warning: snapping must be done, otherwise colinear boundaries are not broken and 
     * topology cannot be built (the same angle). But snapping distance must be very, very 
     * small, otherwise counterclockwise boundaries can appear in areas outside the buffer.
     * I have done some tests on real data (projected) and threshold 1e-8 was not enough,
     * Snapping threshold 1e-7 seems to work. Don't increase until we find example 
     * where it is not sufficient. */
    /* TODO: look at snapping threshold better, calculate some theoretical value to avoid
     * the same angles of lines at nodes, don't forget about LongLat data, probably
     * calculate different threshold for each map, depending on map's bounding box */
    G_message(_("Snapping boundaries...") );
    Vect_snap_lines ( &Out, GV_BOUNDARY, 1e-7, NULL, stderr );

    G_message(_( "Breaking boundaries...") );
    Vect_break_lines ( &Out, GV_BOUNDARY, NULL, stderr );

    G_message(_( "Removing duplicates...") );
    Vect_remove_duplicates ( &Out, GV_BOUNDARY, NULL, stderr );

    /* Dangles and bridges don't seem to be necessary if snapping is small enough. */
    /*
    G_message (  "Removing dangles..." );
    Vect_remove_dangles ( &Out, GV_BOUNDARY, -1, NULL, stderr );

    G_message (  "Removing bridges..." );
    Vect_remove_bridges ( &Out, NULL, stderr );
    */

    G_message(_( "Attaching islands...") );
    Vect_build_partial ( &Out, GV_BUILD_ATTACH_ISLES, stderr );

    /* Calculate new centroids for all areas */
    nareas = Vect_get_num_areas ( &Out );
    Areas = (char *) G_calloc ( nareas+1, sizeof(char) );

    for ( area = 1; area <= nareas; area++ ) {
	G_debug ( 3, "area = %d", area );

/*** BUG *** if dynamic bufcol was used, "buffer" will only hold last value ***/
	ret = area_in_buffer ( &In, &Out, area, type, buffer, tolerance );

	if ( ret ) {
	    G_debug ( 3, "  -> in buffer");
	    Areas[area] = 1;
	}
	
	/* Write out centroid (before check if correct, so that it isd visible for debug) */
        if ( debug == DEBUG_CLEAN ) {
            double x, y;
	    ret = Vect_get_point_in_area ( &Out, area, &x, &y );
	    if ( ret < 0 ) {
		G_warning (_("Cannot calculate area centroid"));
		continue;
	    }
	    Vect_reset_cats ( Cats );
	    if ( Areas[area] ) 
	        Vect_cat_set (Cats, 1, 1 );
	    
	    Vect_reset_line ( Points );
	    Vect_append_point ( Points, x, y, 0. );
	    Vect_write_line ( &Out, GV_CENTROID, Points, Cats );
	}
    }
    
    if ( debug == DEBUG_CLEAN ) {
        stop ( &In, &Out );
	exit (EXIT_SUCCESS);
    }

    /* Make a list of boundaries to be deleted (both sides inside) */
    nlines = Vect_get_num_lines ( &Out );
    G_debug ( 3, "nlines = %d", nlines );
    Lines = (char *) G_calloc ( nlines+1, sizeof(char) );

    for ( line = 1; line <= nlines; line++ ) {
	int j, side[2], areas[2];
	
	G_debug ( 3, "line = %d", line );

	if ( !Vect_line_alive ( &Out, line ) ) continue;
	
	Vect_get_line_areas ( &Out, line, &side[0], &side[1] );

	for ( j = 0; j < 2; j++ ) { 
	    if ( side[j] == 0 ) { /* area/isle not build */
		areas[j] = 0;
	    } else if ( side[j] > 0 ) { /* area */
		areas[j] = side[j];
	    } else { /* < 0 -> island */
		areas[j] = Vect_get_isle_area ( &Out, abs ( side[j] ) );
	    }
	}
    
	G_debug ( 3, " areas = %d , %d -> Areas = %d, %d", areas[0], areas[1], 
				Areas[areas[0]], Areas[areas[1]] );
	if ( Areas[areas[0]] && Areas[areas[1]] ) 
	    Lines[line] = 1;
    }
    G_free( Areas );
    
    /* Delete boundaries */
    for ( line = 1; line <= nlines; line++ ) {
	if ( Lines[line] ) {
	    G_debug ( 3, " delete line %d", line );
	    Vect_delete_line ( &Out, line );
	}
    }

    G_free( Lines );

    /* Create new centroids */
    Vect_reset_cats ( Cats );
    Vect_cat_set (Cats, 1, 1 );
    nareas = Vect_get_num_areas ( &Out );
    for ( area = 1; area <= nareas; area++ ) {
        double x, y;
	G_debug ( 3, "area = %d", area );

	if ( !Vect_area_alive ( &Out, area ) ) continue;

/*** BUG *** if dynamic bufcol was used, "buffer" will only hold last value ***/	
	ret = area_in_buffer ( &In, &Out, area, type, buffer, tolerance );

	if ( ret ) {
	    ret = Vect_get_point_in_area ( &Out, area, &x, &y );
	    if ( ret < 0 ) {
		G_warning (_("Cannot calculate area centroid"));
		continue;
	    }

	    Vect_reset_line ( Points );
	    Vect_append_point ( Points, x, y, 0. );
	    Vect_write_line ( &Out, GV_CENTROID, Points, Cats );
	}
    }

    G_message(_("Attaching centroids...") );
    Vect_build_partial ( &Out, GV_BUILD_CENTROIDS, stderr );
    
    stop ( &In, &Out );
    exit(EXIT_SUCCESS);
}

