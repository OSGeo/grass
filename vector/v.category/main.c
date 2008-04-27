/* ***************************************************************
 * *
 * * MODULE:       v.category
 * * 
 * * AUTHOR(S):    Radim Blazek
 * *               
 * * PURPOSE:      Category manipulations
 * *               
 * * COPYRIGHT:    (C) 2001-2008 by the GRASS Development Team
 * *
 * *               This program is free software under the 
 * *               GNU General Public License (>=v2). 
 * *               Read the file COPYING that comes with GRASS
 * *               for details.
 * *
 * **************************************************************/
#include <stdlib.h> 
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/Vect.h>

#define O_ADD  1
#define O_DEL  2
#define O_REP  3
#define O_PRN  4
#define O_SUM  5
#define O_CHFIELD 6
#define O_TYPE_REP 7 /* report number of features for each type */

#define FRTYPES 7  /* number of field report types */

#define FR_POINT    0
#define FR_LINE     1
#define FR_BOUNDARY 2
#define FR_CENTROID 3
#define FR_AREA     4
#define FR_UNKNOWN  5
#define FR_ALL      6

typedef struct {
    int field;
    char *table;
    int count[FRTYPES];
    int min[FRTYPES], max[FRTYPES];
} FREPORT;
    
int 
main (int argc, char *argv[])
{
	struct Map_info In, Out;
	static struct line_pnts *Points;
	struct line_cats *Cats;
	struct field_info *Fi;
	struct cat_list *Clist;
        int    i, j, ret, option, otype, type, with_z, step, id;
	int    n_areas, centr, new_centr;
	double x, y;
	int    cat, ocat, *fields, nfields, field;
	struct GModule *module;
	struct Option *in_opt, *out_opt, *option_opt, *type_opt;
	struct Option *cat_opt, *field_opt, *step_opt, *id_opt;
	struct Flag *shell;
	FREPORT **freps;
	int nfreps, rtype, fld;

	module = G_define_module();
	module->keywords = _("vector, category");
	module->description = 
	    _("Attach, delete or report vector categories to map geometry.");

	in_opt = G_define_standard_option(G_OPT_V_INPUT);
	out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
	out_opt->required = NO;
	type_opt = G_define_standard_option(G_OPT_V_TYPE);
	
	option_opt = G_define_option();
	option_opt->key = "option";
	option_opt->type =  TYPE_STRING;
	option_opt->required = NO;
	option_opt->multiple = NO;
	option_opt->options = "add,del,chlayer,sum,report,print";
	option_opt->answer = "add";
        option_opt->description = _("Action to be done");
	option_opt->descriptions = _("add;add a new category;"
				     "del;delete category;"
				     "chlayer;change layer number (e.g. layer=3,1 changes layer 3 to layer 1);"
				     "sum;add the value specified by cat option to the current category value;"
				     "report;print report (statistics), in shell style: layer type count min max;"
				     "print;print category values, more cats in the same layer are separated by '/'");

	cat_opt = G_define_standard_option(G_OPT_V_CAT);
	cat_opt->answer = "1";
	
	field_opt = G_define_standard_option(G_OPT_V_FIELD);
	field_opt->answer = "1";
	field_opt->multiple = YES;

	step_opt = G_define_option();
	step_opt->key = "step";
	step_opt->type =  TYPE_INTEGER;
	step_opt->required = NO;
	step_opt->multiple = NO;
	step_opt->answer = "1";
        step_opt->description = _("Category increment");

	id_opt = G_define_standard_option(G_OPT_V_CAT);
	id_opt->multiple = YES;
	id_opt->label = _("Feature id(s)");
	id_opt->description = _("By default all ids are processed");
	id_opt->key = "id";

	shell = G_define_flag();
        shell->key         = 'g';
        shell->label       = _("Shell script style, currently only for report");
	shell->description = _("Format: layer type count min max");

	G_gisinit(argv[0]);

        if (G_parser (argc, argv))
	    exit(EXIT_FAILURE); 
	
	/* read options */
	option = 0;
        switch ( option_opt->answer[0] ) {
	    case ( 'a' ):
		option = O_ADD;
		break;
	    case ( 'd' ):
		option = O_DEL;
		break;
	    case ( 'c' ):
		option = O_CHFIELD;
		G_warning(_("Database connection and attribute tables for concerned layers are not changed"));
		break;
	    case ( 's' ):
		option = O_SUM;
		break;
	    case ( 'r' ):
		option = O_REP;
		break;
	    case ( 'p' ):
		option = O_PRN;
		break;
	}

	cat = atoi( cat_opt->answer );
	step = atoi( step_opt->answer );
	otype = Vect_option_to_types(type_opt);

	/* collect ids */
	if (id_opt->answer) {
	    Clist = Vect_new_cat_list ();
	    Clist->field = atoi (field_opt->answer);
	    ret = Vect_str_to_cat_list (id_opt->answer, Clist);
	    if (ret > 0) {
	        G_warning (_("%d errors in id option"), ret);
	    }
	}
	else {
	    Clist = NULL;
	}

	/* read fields */
	i = 0; nfields = 0;
	while (field_opt->answers[i]) { nfields++; i++; }
	fields = (int *) G_malloc ( nfields * sizeof(int) );
	i = 0;
	while (field_opt->answers[i]) {
	    fields[i] = atoi( field_opt->answers[i] );
	    i++;
	}
	if ( nfields > 1 && option != O_PRN && option != O_CHFIELD )
	    G_fatal_error ( _("Too many layers for this operation") );
	
	if ( nfields != 2 && option == O_CHFIELD )
	    G_fatal_error ( _("2 layers must be specified") );
	
	if ( (option != O_REP) && (option != O_PRN) ) {
	    if (out_opt->answer == NULL) {
	        G_fatal_error ( _("Output vector wasn't entered") );
	    }
	    Vect_check_input_output_name ( in_opt->answer, out_opt->answer, GV_FATAL_EXIT );
	}
	
        Points = Vect_new_line_struct ();
	Cats = Vect_new_cats_struct ();

	/* open input vector */
        Vect_set_open_level (2); 
	Vect_open_old (&In, in_opt->answer, ""); 

	/* open output vector if needed */
	if (option == O_ADD || option == O_DEL || option == O_CHFIELD || option == O_SUM) {
	    with_z = In.head.with_z;
	
	    Vect_set_fatal_error (GV_FATAL_PRINT);
	    if (0 > Vect_open_new (&Out, out_opt->answer, with_z)) {
	         Vect_close (&In);
	         exit (EXIT_FAILURE);
	    }

	    Vect_copy_head_data (&In, &Out);
	    Vect_hist_copy (&In, &Out);
	    Vect_hist_command ( &Out );
        }

	id = 0;

        switch ( option) {	
	    case (O_ADD):	  
		/* Lines */
	        while ( (type = Vect_read_next_line (&In, Points, Cats)) > 0)
	          {
		      id++;
		      if ( type & otype && (!Clist || 
					    (Clist && Vect_cat_in_cat_list(id, Clist) == TRUE))) {
			  if( (Vect_cat_get (Cats, fields[0], &ocat)) == 0)
			  {
			      Vect_cat_set (Cats, fields[0], cat);
			      cat += step;
			  }
		      }	   
		      Vect_write_line ( &Out, type, Points, Cats );
	          }
		/* Areas */
		if ( otype & GV_AREA ) {
		    n_areas = Vect_get_num_areas ( &In );
		    new_centr = 0;
		    for ( i = 1; i <= n_areas; i++ ) {
			centr = Vect_get_area_centroid ( &In, i );
			if ( centr > 0 ) continue; /* Centroid exists and may be processed as line */
			ret = Vect_get_point_in_area ( &In, i, &x, &y );
			if ( ret < 0 ) {
			    G_warning (_("Unable to calculate area centroid"));
			    continue;
			}
			Vect_reset_line ( Points );
			Vect_reset_cats ( Cats );
			Vect_append_point ( Points, x, y, 0.0 );
			Vect_cat_set (Cats, fields[0], cat);
			cat += step;
			Vect_write_line ( &Out, GV_CENTROID, Points, Cats );
			new_centr++;
		    }
		    G_message (_("%d new centroids placed in output map"), new_centr );
		}
		break;
		
	    case (O_DEL):	  
	        while ( (type = Vect_read_next_line (&In, Points, Cats)) > 0)
	          {
		    id++;
	            if ( type & otype  && (!Clist || 
					   (Clist && Vect_cat_in_cat_list(id, Clist) == TRUE)))
	               {
                         ret = Vect_cat_del (Cats, fields[0]);
	               }	   
	            Vect_write_line ( &Out, type, Points, Cats );
	          }
		break;

	    case (O_CHFIELD):	  
	        while ( (type = Vect_read_next_line (&In, Points, Cats)) > 0) {
		    id++;
	            if ( type & otype && (!Clist || 
					  (Clist && Vect_cat_in_cat_list(id, Clist) == TRUE))) {
			for ( i = 0 ; i < Cats->n_cats; i++ ) {
			   if ( Cats->field[i] == fields[0] ) {
			       Cats->field[i] = fields[1];
			   }
			}
	            }
	            Vect_write_line ( &Out, type, Points, Cats );
	        }
		break;
		
	    case (O_SUM):	  
	        while ( (type = Vect_read_next_line (&In, Points, Cats)) > 0) {
		    id++;
	            if ( type & otype && (!Clist || 
					  (Clist && Vect_cat_in_cat_list(id, Clist) == TRUE))) {
			for ( i = 0 ; i < Cats->n_cats; i++ ) {
			   if ( Cats->field[i] == fields[0] ) {
			       Cats->cat[i] += cat;
			   }
			}
	            }	   
	            Vect_write_line ( &Out, type, Points, Cats );
	        }
		break;

	    case (O_REP):	  
		nfreps = 0;
		freps = NULL;
	        while ( (type = Vect_read_next_line (&In, Points, Cats)) > 0)
	          {
		    id++;
		    if (Clist && Vect_cat_in_cat_list(id, Clist) == FALSE)
			continue;

                    switch (type)
		      {
                        case (GV_POINT):
			    rtype = FR_POINT;
			    break;
                        case (GV_LINE):
			    rtype = FR_LINE;
			    break;
                        case (GV_BOUNDARY):
			    rtype = FR_BOUNDARY;
			    break;
                        case (GV_CENTROID):
			    rtype = FR_CENTROID;
			    break;
			default:
			    rtype = FR_UNKNOWN;
		      }
		    
		    for (i=0; i < Cats->n_cats; i++)
                      {
                        field = Cats->field[i];
                        cat = Cats->cat[i];
                        
			
			ret = FALSE;
		        for (j=0; j < nfreps; j++)
			  {
                            if ( freps[j]->field == field )
			      {
				fld = j;      
				ret = TRUE;      
				break;
			      }
			  }
			if ( !ret ) /* field report doesn't exist */
		          {
			    nfreps++; 	    
                            freps = (FREPORT **) G_realloc ( freps, nfreps * sizeof (FREPORT *));
                            fld = nfreps - 1;
                            freps[fld] = (FREPORT *) G_calloc ( 1, sizeof (FREPORT));
                            freps[fld]->field = field;
			    if ((Fi = Vect_get_field (&In, field)) != NULL) {
			      freps[fld]->table = G_store (Fi->table);
			    }
			    else {
			      freps[fld]->table = '\0';
			    }
			  }
			
                        freps[fld]->count[rtype]++;
                        freps[fld]->count[FR_ALL]++;
			
			if ( (freps[fld]->min[rtype] == 0) || freps[fld]->min[rtype] > cat)
		            freps[fld]->min[rtype] = cat;
			
			if ( (freps[fld]->max[rtype] == 0) || freps[fld]->max[rtype] < cat)
		            freps[fld]->max[rtype] = cat;
			
			if ( (freps[fld]->min[FR_ALL] == 0) || freps[fld]->min[FR_ALL] > cat)
		            freps[fld]->min[FR_ALL] = cat;
			
			if ( (freps[fld]->max[FR_ALL] == 0) || freps[fld]->max[FR_ALL] < cat)
		            freps[fld]->max[FR_ALL] = cat;
		      }
	          }
                for (i=0; i < nfreps; i++)
		  {
		    if ( shell->answer ) {
			if ( freps[i]->count[FR_POINT] > 0 )
			    fprintf (stdout, "%d point %d %d %d\n", freps[i]->field, freps[i]->count[FR_POINT],
				     freps[i]->min[FR_POINT], freps[i]->max[FR_POINT]);
			
			if ( freps[i]->count[FR_LINE] > 0 )
			    fprintf (stdout, "%d line %d %d %d\n", freps[i]->field, freps[i]->count[FR_LINE], 
				     freps[i]->min[FR_LINE], freps[i]->max[FR_LINE]);
			
			if ( freps[i]->count[FR_BOUNDARY] > 0 )
			    fprintf (stdout, "%d boundary %d %d %d\n", freps[i]->field, 
				     freps[i]->count[FR_BOUNDARY], freps[i]->min[FR_BOUNDARY], 
				     freps[i]->max[FR_BOUNDARY]);
			
			if ( freps[i]->count[FR_CENTROID] > 0 )
			    fprintf (stdout, "%d centroid %d %d %d\n", freps[i]->field, 
				     freps[i]->count[FR_CENTROID], freps[i]->min[FR_CENTROID], 
				     freps[i]->max[FR_CENTROID]);
			
			if ( freps[i]->count[FR_AREA] > 0 )
			    fprintf (stdout, "%d area %d %d %d\n", freps[i]->field, 
				     freps[i]->count[FR_AREA], freps[i]->min[FR_AREA], 
				     freps[i]->max[FR_AREA]);
			
			if ( freps[i]->count[FR_ALL] > 0 )
			    fprintf (stdout, "%d all %d %d %d\n", freps[i]->field, 
				     freps[i]->count[FR_ALL], freps[i]->min[FR_ALL], 
				     freps[i]->max[FR_ALL]);
		    } else {
		        if (freps[i]->table != '\0') {
			    G_message("%s: %d/%s", _("Layer/table"),
				      freps[i]->field, freps[i]->table);
		        }
		        else {
			    G_message ("%s: %d", _("Layer"),
				       freps[i]->field);
		        }
			fprintf (stdout, "type       count        min        max\n");
			fprintf (stdout, "point    %7d %10d %10d\n", 
					freps[i]->count[FR_POINT],
					freps[i]->min[FR_POINT],
					freps[i]->max[FR_POINT]);
			fprintf (stdout, "line     %7d %10d %10d\n", 
					freps[i]->count[FR_LINE],
					freps[i]->min[FR_LINE],
					freps[i]->max[FR_LINE]);
			fprintf (stdout, "boundary %7d %10d %10d\n", 
					freps[i]->count[FR_BOUNDARY],
					freps[i]->min[FR_BOUNDARY],
					freps[i]->max[FR_BOUNDARY]);
			fprintf (stdout, "centroid %7d %10d %10d\n", 
					freps[i]->count[FR_CENTROID],
					freps[i]->min[FR_CENTROID],
					freps[i]->max[FR_CENTROID]);
			fprintf (stdout, "area     %7d %10d %10d\n", 
					freps[i]->count[FR_AREA],
					freps[i]->min[FR_AREA],
					freps[i]->max[FR_AREA]);
			fprintf (stdout, "all      %7d %10d %10d\n", 
					freps[i]->count[FR_ALL],
					freps[i]->min[FR_ALL],
					freps[i]->max[FR_ALL]);
		    }
		  }
		break;
		
	    case (O_PRN):
	        while ( (type = Vect_read_next_line (&In, Points, Cats)) > 0) {
		    id++;
		    int has = 0;
		    
	            if ( !(type & otype) )
			continue;

		    if (Clist && Vect_cat_in_cat_list(id, Clist) == FALSE)
			continue;

		    /* Check if the line has at least one cat */
		    for (i=0; i < nfields; i++) {
			for (j=0; j < Cats->n_cats; j++) {
			    if ( Cats->field[j] == fields[i] ) {
				has  = 1;
				break;
			    }
			}
		    }

		    if ( !has ) continue;

		    for (i=0; i < nfields; i++) {
			int first = 1;
		        
			if ( i > 0 ) fprintf (stdout, "|" );
		        for (j=0; j < Cats->n_cats; j++) {
			    if ( Cats->field[j] == fields[i] ) {
				if ( !first ) fprintf (stdout, "/");
		                fprintf (stdout, "%d", Cats->cat[j] );
				first = 0;
			    }
			}
		    }
	            fprintf (stdout, "\n" );
		}
		break;
	}
	
	if (option == O_ADD || option == O_DEL || option == O_CHFIELD || option == O_SUM) {
	    Vect_copy_tables ( &In, &Out, 0 );
	    if (G_verbose() > G_verbose_min()) {
		Vect_build (&Out, stderr);
	    }
	    else {
		Vect_build (&Out, NULL);
	    }
	    Vect_close (&Out);
	}
	Vect_close (&In);

	exit(EXIT_SUCCESS);
}
