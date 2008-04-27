/****************************************************************************
 *
 * MODULE:       v.select - select features from one map by features in another map.
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Glynn Clements <glynn gclements.plus.com>, Markus Neteler <neteler itc.it>
 * PURPOSE:      
 * COPYRIGHT:    (C) 2003-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

#define OP_OVERLAP 0

/* Add all elements of area A to the list */
void add_aarea ( struct Map_info *In, int aarea, int *ALines )
{
    int i, j, aline, naisles, aisle, acentroid;
    static struct ilist *BoundList = NULL;

    if ( !BoundList )
        BoundList = Vect_new_list ();

    acentroid = Vect_get_area_centroid ( In, aarea );
    ALines[acentroid] = 1;

    Vect_get_area_boundaries ( In, aarea, BoundList );
    for ( i = 0; i < BoundList->n_values; i++ ) {
	aline = abs(BoundList->value[i]);
	ALines[aline] = 1;
    }

    naisles = Vect_get_area_num_isles (  In, aarea );

    for ( j = 0; j < naisles; j++ ) {
	aisle = Vect_get_area_isle ( In, aarea, j );

	Vect_get_isle_boundaries ( In, aisle, BoundList );
	for ( i = 0; i < BoundList->n_values; i++ ) {
	    aline = abs(BoundList->value[i]);
	    ALines[aline] = 1;
	}
    }
}
/* Returns 1 if line1 from Map1 overlaps area2 from Map2,
 *         0 otherwise */
int line_overlap_area ( struct Map_info *LMap, int line, struct Map_info *AMap, int area ) 
{
    int i, nisles, isle;
    static struct line_pnts *LPoints = NULL;
    static struct line_pnts *APoints = NULL;

    G_debug (4, "line_overlap_area line = %d area = %d", line, area);
    
    if ( !LPoints ) {
        LPoints = Vect_new_line_struct();
        APoints = Vect_new_line_struct();
    }

    /* Read line coordinates */    
    Vect_read_line (LMap, LPoints, NULL, line);

    /* Try if any of line vertices is within area */
    for (i = 0; i < LPoints->n_points; i++ ) {
	if ( Vect_point_in_area(AMap, area, LPoints->x[i], LPoints->y[i]) ) {
            G_debug (4, "  -> line vertex inside area");
	    return 1;
	}
    }

    /* Try intersections of line with area/isles boundary */
    /* Outer boundary */
    Vect_get_area_points ( AMap, area, APoints );

    if ( Vect_line_check_intersection(LPoints, APoints, 0) ) {
        G_debug (4, "  -> line intersects outer area boundary");
	return 1;
    }

    nisles = Vect_get_area_num_isles ( AMap, area );

    for ( i = 0; i < nisles; i++ ) {
	isle = Vect_get_area_isle ( AMap, area, i );
	Vect_get_isle_points ( AMap, isle, APoints );

	if ( Vect_line_check_intersection(LPoints, APoints, 0) ) {
            G_debug (4, "  -> line intersects area island boundary");
	    return 1;
	}
    }
    return 0;
}

int 
main (int argc, char *argv[])
{
    int    i;
    int    input, operator;
    int    aline, nalines;
    int    type[2], field[2];
    int    **cats, *ncats, nfields, *fields;
    char   *mapset[2], *pre[2];
    struct GModule *module;
    struct Option *in_opt[2], *out_opt, *type_opt[2], *field_opt[2], *operator_opt;
    struct Flag *table_flag;
    struct Map_info In[2], Out;
    struct field_info *IFi, *OFi;
    struct line_pnts *APoints, *BPoints;
    struct line_cats *ACats, *BCats;
    int    *ALines;  /* List of lines: 0 do not output, 1 - write to output */
    struct ilist *List, *TmpList, *BoundList;

    G_gisinit (argv[0]);

    pre[0] ="a";
    pre[1] ="b";

    module = G_define_module();
    module->keywords = _("vector");
    module->description = _("Select features from ainput by features from binput");

    in_opt[0] = G_define_standard_option(G_OPT_V_INPUT);
    in_opt[0]->key = "ainput";

    type_opt[0] = G_define_standard_option(G_OPT_V_TYPE) ;
    type_opt[0]->key = "atype";

    field_opt[0] = G_define_standard_option(G_OPT_V_FIELD);
    field_opt[0]->key = "alayer";
    
    in_opt[1] = G_define_standard_option(G_OPT_V_INPUT);
    in_opt[1]->key = "binput";

    type_opt[1] = G_define_standard_option(G_OPT_V_TYPE) ;
    type_opt[1]->key = "btype";

    field_opt[1] = G_define_standard_option(G_OPT_V_FIELD);
    field_opt[1]->key = "blayer";
    
    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    operator_opt = G_define_option();
    operator_opt->key = "operator";
    operator_opt->type = TYPE_STRING;
    operator_opt->required = NO;
    operator_opt->multiple = NO;
    operator_opt->options = "overlap";
    operator_opt->answer = "overlap";
    operator_opt->description = _("Operator defines required relation between features. "
	"A feature is written to output if the result of operation 'ainput operator binput' is true. "
	"An input feature is considered to be true, if category of given layer is defined.\n"
	"\t overlap: features partially or completely overlap");

    table_flag = G_define_flag ();
    table_flag->key             = 't';
    table_flag->description     = _("Do not create attribute table");

    if (G_parser (argc, argv))
       exit(EXIT_FAILURE);

    if ( operator_opt->answer[0] == 'o' ) operator = OP_OVERLAP;

    for ( input = 0; input < 2; input++ ) {
	type[input] = Vect_option_to_types (type_opt[input]);
        field[input] = atoi(field_opt[input]->answer);

	Vect_check_input_output_name ( in_opt[input]->answer, out_opt->answer, GV_FATAL_EXIT );

	if ((mapset[input] = G_find_vector2 (in_opt[input]->answer, NULL)) == NULL) {
	    G_fatal_error (_("Vector map <%s> not found"), in_opt[input]->answer);
	}

	Vect_set_open_level (2);
	Vect_open_old (&(In[input]), in_opt[input]->answer, mapset[input] );
    }
    
    /* Read field info */
    IFi = Vect_get_field(&(In[0]), field[0]);
    
    APoints = Vect_new_line_struct();
    BPoints = Vect_new_line_struct();
    ACats = Vect_new_cats_struct();
    BCats = Vect_new_cats_struct();
    List = Vect_new_list ();
    TmpList = Vect_new_list ();
    BoundList = Vect_new_list ();

    /* Open output */
    Vect_open_new (&Out, out_opt->answer, Vect_is_3d ( &(In[0]) ) );
    Vect_set_map_name ( &Out, "Output from v.select");
    Vect_set_person ( &Out, G_whoami ());
    Vect_copy_head_data (&(In[0]), &Out);
    Vect_hist_copy (&(In[0]), &Out);
    Vect_hist_command ( &Out );

    nalines = Vect_get_num_lines ( &(In[0]) );
    
    /* Alloc space for input lines array */
    ALines = (int *) G_calloc ( nalines+1, sizeof(int) );
    
    /* Lines in A. Go through all lines and mark those that meets condition */
    if ( type[0] & (GV_POINTS | GV_LINES) ) {
	G_message (_("Processing ainput lines ...") );

	for ( aline = 1; aline <= nalines; aline++ ) {
	    BOUND_BOX abox;
	    
	    G_debug (3, "aline = %d", aline);
	    G_percent ( aline, nalines, 1 ); /* must be before any continue */
	    
	    /* Check category */
	    if ( Vect_get_line_cat(&(In[0]), aline, field[0]) < 0 ) continue;
	    
	    /* Read line and check type */
	    if ( !(Vect_read_line(&(In[0]), APoints, NULL, aline) & type[0]) ) continue;

	    Vect_get_line_box ( &(In[0]), aline, &abox );
	    abox.T = PORT_DOUBLE_MAX; abox.B = -PORT_DOUBLE_MAX;

	    /* Check if this line overlaps any feature in B */

	    /* x Lines in B */
	    if ( type[1] & (GV_POINTS | GV_LINES) ) {
		int i;
		int found = 0;

		/* Lines */
		Vect_select_lines_by_box ( &(In[1]), &abox, type[1], List);
		for (i = 0; i < List->n_values; i++) {
		    int bline;
		    
		    bline = List->value[i];
		    G_debug (3, "  bline = %d", bline);
		    
		    /* Check category */
		    if ( !Vect_get_line_cat(&(In[1]), bline, field[1]) < 0 ) continue;

		    Vect_read_line ( &(In[1]), BPoints, NULL, bline);

		    if ( Vect_line_check_intersection ( APoints, BPoints, 0 ) ) {
			found = 1;
			break;
		    }
		}
		
		if ( found ) {
		    ALines[aline] = 1;
		    continue; /* Go to next A line */
		}
	    }

	    /* x Areas in B. */
	    if ( type[1] & GV_AREA ) {
		int i;

		Vect_select_areas_by_box ( &(In[1]), &abox, List);
		for (i = 0; i < List->n_values; i++) {
		    int barea;

		    barea = List->value[i];
		    G_debug (3, "  barea = %d", barea);

		    if ( Vect_get_area_cat(&(In[1]), barea, field[1]) < 0 ) continue;

		    if ( line_overlap_area(&(In[0]), aline, &(In[1]), barea) ) {
		        ALines[aline] = 1;
			break;
		    }
		}
	    }
	}
    }
    
    /* Areas in A. */
    if ( type[0] & GV_AREA ) {
        int  aarea, naareas;

	G_message ( _("Processing ainput areas ...") );

	naareas = Vect_get_num_areas (  &(In[0]) );

	for ( aarea = 1; aarea <= naareas; aarea++ ) {
            int  i;
	    BOUND_BOX abox;
	    
	    G_percent ( aarea, naareas, 1 ); /* must be before any continue */

	    if ( Vect_get_area_cat(&(In[0]), aarea, field[0]) < 0 ) continue;
	    Vect_get_area_box ( &(In[0]), aarea, &abox );
	    abox.T = PORT_DOUBLE_MAX; abox.B = -PORT_DOUBLE_MAX;

	    /* x Lines in B */
	    if ( type[1] & (GV_POINTS | GV_LINES) ) {
		Vect_select_lines_by_box ( &(In[1]), &abox, type[1], List);

		for (i = 0; i < List->n_values; i++) {
		    int bline;
		    
		    bline = List->value[i];
		
		    if ( Vect_get_line_cat(&(In[1]), bline, field[1]) < 0 ) continue;

		    if ( line_overlap_area(&(In[1]), bline, &(In[0]), aarea) ) {
			add_aarea ( &(In[0]), aarea, ALines );
			continue;
		    }
		}
	    }

	    /* x Areas in B */
	    if ( type[1] & GV_AREA ) {
		int naisles;
		int found = 0;

		/* List of areas B */
		
		/* Make a list of features forming area A */
		Vect_reset_list ( List );

		Vect_get_area_boundaries ( &(In[0]), aarea, BoundList );
		for ( i = 0; i < BoundList->n_values; i++ ) {
		    Vect_list_append ( List, abs(BoundList->value[i]) );
		}

		naisles = Vect_get_area_num_isles (  &(In[0]), aarea );

		for ( i = 0; i < naisles; i++ ) {
		    int j, aisle;

		    aisle = Vect_get_area_isle ( &(In[0]), aarea, i );

		    Vect_get_isle_boundaries ( &(In[0]), aisle, BoundList );
		    for ( j = 0; j < BoundList->n_values; j++ ) {
			Vect_list_append ( List, BoundList->value[j] );
		    }
		}

		Vect_select_areas_by_box ( &(In[1]), &abox, TmpList);

		for (i = 0; i < List->n_values; i++) {
		    int j, aline;
			
		    aline = abs ( List->value[i] );

		    for (j = 0; j < TmpList->n_values; j++) {
			int barea, bcentroid;
		    
			barea = TmpList->value[j];
			G_debug (3, "  barea = %d", barea);
			
			if ( Vect_get_area_cat(&(In[1]), barea, field[1]) < 0 ) continue;
		
			/* Check if any centroid of area B is in area A.
		         * This test is important in if area B is completely within area A */
			bcentroid = Vect_get_area_centroid ( &(In[1]), barea );
	                Vect_read_line ( &(In[1]), BPoints, NULL, bcentroid);

	                if ( Vect_point_in_area ( &(In[0]), aarea, BPoints->x[0], BPoints->y[0]) ) {
			    found = 1;
			    break;
			}

		        /* Check intersectin of lines from List with area B */
			if ( line_overlap_area(&(In[0]), aline, &(In[1]), barea) ) {
			    found = 1;
			    break;
			}
		    }
		    if ( found ) {
                        add_aarea ( &(In[0]), aarea, ALines );
			break;
		    }
		}
	    }
	}
    }
        
    Vect_close ( &(In[1]) ); 

    /* Write lines */
    nfields = Vect_cidx_get_num_fields ( &(In[0]) );
    cats = (int **) G_malloc ( nfields * sizeof(int *) );
    ncats = (int *) G_malloc ( nfields * sizeof(int) );
    fields = (int *) G_malloc ( nfields * sizeof(int) );
    for ( i = 0; i < nfields; i++ ) {
	ncats[i] = 0;
	cats[i] = (int *) G_malloc ( Vect_cidx_get_num_cats_by_index(&(In[0]),i) * sizeof(int) );
	fields[i] = Vect_cidx_get_field_number ( &(In[0]), i );
    }
    for ( aline = 1; aline <= nalines; aline++ ) {
	int atype;

        G_debug (4, "aline = %d ALines[aline] = %d", aline, ALines[aline]);

	if ( !(ALines[aline]) ) continue;
	
	atype = Vect_read_line ( &(In[0]), APoints, ACats, aline);
        Vect_write_line ( &Out, atype, APoints, ACats );
	
        if ( !(table_flag->answer) && (IFi != NULL) ) {
	    for ( i = 0; i < ACats->n_cats; i++ ) {
		int f, j; 
		for ( j = 0; j < nfields; j++ ) { /* find field */
	    	    if ( fields[j] == ACats->field[i] ) {
			f = j;
			break;
		    }
		}
		cats[f][ncats[f]] = ACats->cat[i];
		ncats[f]++;
	    }
	}
    }

    /* Copy tables */
    if ( !(table_flag->answer) ) {
	int ttype, ntabs=0;
	
	G_message ( _("Writing attributes ...") );

	/* Number of output tabs */
	for ( i = 0; i < Vect_get_num_dblinks ( &(In[0]) ); i++ ) {
	    int f, j;
	    
	    IFi = Vect_get_dblink ( &(In[0]), i );
	    
	    for ( j = 0; j < nfields; j++ ) { /* find field */
	    	if ( fields[j] == IFi->number ) {
		    f = j;
		    break;
		}
	    }
	    if ( ncats[f] > 0 ) ntabs++;
	}
	
	if ( ntabs > 1 )
	    ttype = GV_MTABLE;
	else 
	    ttype = GV_1TABLE;
	
	for ( i = 0; i < nfields; i++ ) {
	    int ret;

	    if ( fields[i] == 0 ) continue;
	
	    G_message ( _("Layer %d"), fields[i] );

	    /* Make a list of categories */
	    IFi = Vect_get_field ( &(In[0]), fields[i] );
	    if ( !IFi ) { /* no table */
		G_message ( "No table." );
		continue;
	    }
	    
	    OFi = Vect_default_field_info ( &Out, IFi->number, IFi->name, ttype );

	    ret = db_copy_table_by_ints ( IFi->driver, IFi->database, IFi->table,
				  OFi->driver, Vect_subst_var(OFi->database,&Out), OFi->table,
				  IFi->key, cats[i], ncats[i] );

	    if ( ret == DB_FAILED ) {
		G_warning ( _("Cannot copy table") );
	    } else {
		Vect_map_add_dblink ( &Out, OFi->number, OFi->name, OFi->table, 
				      IFi->key, OFi->database, OFi->driver);
	    }
	    G_done_msg("");
	}
    }

    Vect_close ( &(In[0]) ); 

    Vect_build (&Out, stderr); 
    Vect_close (&Out);
    
    exit (EXIT_SUCCESS);
}




