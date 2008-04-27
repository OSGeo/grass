#include <stdio.h>
#include <stdlib.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "global.h"

static int srch(); 

int 
update (struct Map_info *Map)
{
    int    i, *catexst, *cex, upd, fcat;
    char   buf1[2000], buf2[2000], left[20], right[20];
    struct field_info *Fi;
    dbString stmt; 
    dbDriver *driver;

    vstat.dupl=0;     
    vstat.exist=0;
    vstat.notexist=0;    
    vstat.update=0;
    vstat.error=0;

    db_init_string (&stmt);	

    if ( (Fi = Vect_get_field ( Map, options.field)) == NULL)
         G_fatal_error(_("Database connection not defined for layer %d. Use v.db.connect first."), options.field);

    /* Open driver */
    driver = db_start_driver_open_database ( Fi->driver, Fi->database );
    if ( driver == NULL ) {
        G_fatal_error (_("Unable to open database <%s> by driver <%s>"), Fi->database, Fi->driver );
    }
    
    db_begin_transaction ( driver );
    
    /* select existing categories to array (array is sorted) */
    vstat.select = db_select_int( driver, Fi->table, Fi->key, NULL, &catexst);
    
    /* create beginning of stmt */
    switch (options.option) {
        case O_CAT:	 
	    sprintf (buf1, "insert into %s ( %s ) values ", Fi->table, Fi->key);
            break;	
        case O_COUNT:
        case O_LENGTH:
	case O_AREA:
	case O_QUERY:
	case O_COMPACT:
	case O_FD:
	case O_PERIMETER:
        case O_SLOPE:
        case O_SINUOUS:
	    sprintf (buf1, "update %s set %s =", Fi->table, options.col[0]);
            break;
        case O_COOR:
        case O_START:
        case O_END:
        case O_SIDES:
	    sprintf (buf1, "update %s set ", Fi->table);	
            break;
    } 

    /* update */
    G_message (_("Updating database..."));
    for ( i = 0; i < vstat.rcat; i++ ) {
	G_percent( i+1, vstat.rcat, 1 );
	
	fcat = Values[i].cat;
	if ( fcat < 0 ) continue;
	switch (options.option) {
    	    case O_CAT:	 
	        sprintf (buf2, "%s ( %d )", buf1, Values[i].cat);
        	break;	

    	    case O_COUNT:
	        sprintf (buf2, "%s %d where %s = %d", buf1, Values[i].count1, Fi->key,  Values[i].cat);
		break;

    	    case O_LENGTH:
	    case O_AREA:
	    case O_COMPACT:
	    case O_FD:
	    case O_PERIMETER:
	    case O_SLOPE:
	    case O_SINUOUS:
    		sprintf (buf2, "%s %f where %s = %d", buf1, Values[i].d1, Fi->key,  Values[i].cat);
        	break;

    	    case O_COOR:
    	    case O_START:
    	    case O_END:
		if ( Values[i].count1 > 1 ){
		    G_warning (_("More elements of category [%d], nothing loaded to DB"), Values[i].cat);
		    vstat.dupl++;
		    continue;
		}		
	        if ( Values[i].count1 < 1 ){ /* No points */
		    continue;
		}
		if ( options.col[2] ) {
		    sprintf (buf2, "%s %s = %.15g, %s = %.15g, %s = %.15g where %s = %d", buf1, options.col[0], 
			            Values[i].d1, options.col[1], Values[i].d2, options.col[2], Values[i].d3,
				    Fi->key,  Values[i].cat);    		
		} else {
		    sprintf (buf2, "%s %s = %.15g, %s = %.15g  where %s = %d", buf1, options.col[0], Values[i].d1, 
				options.col[1], Values[i].d2, Fi->key,  Values[i].cat);    		
		}
		break;

	    case O_SIDES:
		if ( Values[i].count1 == 1 ) {
		    if ( Values[i].i1 >= 0 )
			sprintf ( left, "%d", Values[i].i1 );
		    else
			sprintf ( left, "-1" ); /* NULL, no area/cat */
		} else if ( Values[i].count1 > 1 ) {
		    sprintf ( left, "null" );
		} else { /* Values[i].count1 == 0 */
		    /* It can be OK if the category is assigned to an element
			type which is not GV_BOUNDARY */
		    /* -> TODO: print only if there is boundary with that cat */	
		    sprintf ( left, "null" );
		}

		if ( Values[i].count2 == 1 ) {
		    if ( Values[i].i2 >= 0 )
			sprintf ( right, "%d", Values[i].i2 );
		    else
			sprintf ( right, "-1" ); /* NULL, no area/cat */
		} else if ( Values[i].count2 > 1 ) {
		    sprintf ( right, "null" );
		} else { /* Values[i].count1 == 0 */
		    sprintf ( right, "null" );
		}

    		sprintf (buf2, "%s %s = %s, %s = %s  where %s = %d", buf1, options.col[0], left, 
			    options.col[1], right, Fi->key,  Values[i].cat);    		

		break;

    	    case O_QUERY:
		if ( Values[i].null ) {
		    sprintf (buf2, "%s null where %s = %d", buf1, Fi->key, Values[i].cat);
		} else { 
		    switch ( vstat.qtype ) {
			case ( DB_C_TYPE_INT ):
			    sprintf (buf2, "%s %d where %s = %d", buf1, Values[i].i1, Fi->key, Values[i].cat);
			    break;
			case ( DB_C_TYPE_DOUBLE ):
			    sprintf (buf2, "%s %f where %s = %d", buf1, Values[i].d1, Fi->key, Values[i].cat);
			    break;
			case ( DB_C_TYPE_STRING ):
			    sprintf (buf2, "%s '%s' where %s = %d", buf1, Values[i].str1, Fi->key, Values[i].cat);
			    break;
			case ( DB_C_TYPE_DATETIME ):
			    sprintf (buf2, "%s '%s' where %s = %d", buf1, Values[i].str1, Fi->key, Values[i].cat);
			    break;
		    }
		}
	} 

	db_set_string (&stmt, buf2);
	
	/* category exist in DB ? */
	cex = (int *) bsearch((void *) &fcat, catexst, vstat.select, sizeof(int), srch);
	
        if ( options.option == O_CAT ){
            if ( cex == NULL ){ /* cat does not exist in DB */
	        upd = 1;
	        vstat.notexist++;
	    } else { /* cat exists in DB */
		G_warning (_("Cat [%d]: row already exists (not inserted)"), fcat);
		upd = 0;
		vstat.exist++;
	    }
	} else { 
	    if ( cex == NULL ){ /* cat does not exist in DB */
		G_warning (_("Cat [%d]: row does not exist (not updated)"), fcat);
		upd = 0;
		vstat.notexist++;
	    } else {  /* cat exists in DB */ 
		upd = 1;
		vstat.exist++;
	    }
	}

	if ( upd == 1 ){
	    if ( options.sql )  {
		fprintf (stdout, "%s\n", db_get_string (&stmt) );
	    } else {
		if ( db_execute_immediate (driver, &stmt) == DB_OK ){
		    vstat.update++;
		} else {    
		    vstat.error++;
		}
	    }
        }
    }
    
    db_commit_transaction ( driver );

    G_free (catexst);	
    db_close_database_shutdown_driver ( driver );
    db_free_string (&stmt);

    return 0;
}

int srch ( const void *pa, const void *pb)
{
    int       *p1 = (int *) pa;
    int       *p2 = (int *) pb;    

    if( *p1 < *p2 ) return -1;
    if( *p1 > *p2) return 1;
    return 0;
}  
