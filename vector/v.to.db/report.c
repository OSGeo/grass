#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "global.h"

int 
report (void)
{
    int i;
    char left[20], right[20];

    switch (options.option) {
        case O_CAT:
	    if (G_verbose() > G_verbose_min())
		fprintf (stdout,"cat\n");
	    for ( i = 0; i < vstat.rcat; i++ )
	        fprintf (stdout, "%d\n", Values[i].cat);
            break;	

        case O_COUNT:
            if ( options.total ) {
                int sum = 0;
		for ( i = 0; i < vstat.rcat; i++ ) {
		    sum += Values[i].count1;
		}
		fprintf (stdout,"total count: %d\n", sum);
            } else { 
		if (G_verbose() > G_verbose_min())
		    fprintf (stdout,"cat|count\n");
		for ( i = 0; i < vstat.rcat; i++ )
		    fprintf (stdout, "%d|%d\n", Values[i].cat, Values[i].count1);
	    }
            break;

	case O_AREA:
            if ( options.total ) {
                double sum = 0.0;
		for ( i = 0; i < vstat.rcat; i++ ) {
		    sum += Values[i].d1;
		}
		fprintf (stdout,"total area: %.15g\n", sum);
            } else { 
		if (G_verbose() > G_verbose_min())
		    fprintf (stdout, "cat|area\n");
		for ( i = 0; i < vstat.rcat; i++ )
		    fprintf (stdout, "%d|%.15g\n", Values[i].cat, Values[i].d1);	
            }
            break;

        case O_COMPACT:
	    if (G_verbose() > G_verbose_min())
		fprintf (stdout,"cat|compact\n");
	    for ( i = 0; i < vstat.rcat; i++ )
		fprintf (stdout, "%d|%.15g\n", Values[i].cat, Values[i].d1);
	    break;

        case O_PERIMETER:
	    if (G_verbose() > G_verbose_min())
		fprintf (stdout,"cat|perimeter\n");
	    for ( i = 0; i < vstat.rcat; i++ )
		fprintf (stdout, "%d|%.15g\n", Values[i].cat, Values[i].d1);
	    break;

        case O_LENGTH:
            if ( options.total ) {
                double sum = 0.0;
		for ( i = 0; i < vstat.rcat; i++ ) {
		    sum += Values[i].d1;
		}
		fprintf (stdout,"total length: %.15g\n", sum);
            } else { 
		if (G_verbose() > G_verbose_min())
		    fprintf (stdout,"cat|length\n");
		for ( i = 0; i < vstat.rcat; i++ )
		    fprintf (stdout, "%d|%.15g\n", Values[i].cat, Values[i].d1);	        
	    }
	    break;
        case O_SLOPE:
	    if (G_verbose() > G_verbose_min())
		fprintf (stdout,"cat|slope\n");
	    for ( i = 0; i < vstat.rcat; i++ )
		fprintf (stdout, "%d|%.15g\n", Values[i].cat, Values[i].d1);


            break;
        case O_SINUOUS:
	    if (G_verbose() > G_verbose_min())
		fprintf (stdout,"cat|sinuous\n");
	    for ( i = 0; i < vstat.rcat; i++ )
		fprintf (stdout, "%d|%.15g\n", Values[i].cat, Values[i].d1);
            break;
        case O_COOR:
        case O_START:
        case O_END:
	    if (G_verbose() > G_verbose_min())
		fprintf (stdout,"cat|x|y|z\n");
	    for ( i = 0; i < vstat.rcat; i++ ) {
		if ( Values[i].count1 == 1 )
	            fprintf (stdout, "%d|%.15g|%.15g|%.15g\n", Values[i].cat, Values[i].d1, Values[i].d2, Values[i].d3);
	    }
            break;

        case O_SIDES:
	    if (G_verbose() > G_verbose_min())
		fprintf (stdout,"cat|left|right\n");
	    for ( i = 0; i < vstat.rcat; i++ ) {
		if ( Values[i].count1 == 1 ) {
		    if ( Values[i].i1 >= 0 )
		        sprintf ( left, "%d", Values[i].i1 );
		    else
		        sprintf ( left, "-1" ); /* NULL, no area/cat */
		} else if ( Values[i].count1 > 1 ) {
		    sprintf ( left, "-" );
		} else { /* Values[i].count1 == 0 */
                    /* It can be OK if the category is assigned to an element
                        type which is not GV_BOUNDARY */
                    /* -> TODO: print only if there is boundary with that cat */
		    sprintf ( left, "-" );
		}

		if ( Values[i].count2 == 1 ) {
		    if ( Values[i].i2 >= 0 )
		        sprintf ( right, "%d", Values[i].i2 );
		    else
		        sprintf ( right, "-1" ); /* NULL, no area/cat */
		} else if ( Values[i].count2 > 1 ) {
		    sprintf ( right, "-" );
		} else { /* Values[i].count1 == 0 */
		    sprintf ( right, "-" );
		}

	        fprintf ( stdout, "%d|%s|%s\n", Values[i].cat, left, right );
	    }
            break;

        case O_QUERY:
	    if (G_verbose() > G_verbose_min())
		fprintf (stdout,"cat|query\n");
	    for ( i = 0; i < vstat.rcat; i++ ) {
		if ( Values[i].null ) {
	            fprintf (stdout, "%d|-\n", Values[i].cat);
		} else {
		    switch ( vstat.qtype ) {
			case ( DB_C_TYPE_INT ):
	                    fprintf (stdout, "%d|%d\n", Values[i].cat, Values[i].i1);	        
			    break;
			case ( DB_C_TYPE_DOUBLE ):
	                    fprintf (stdout, "%d|%15g\n", Values[i].cat, Values[i].d1);       
			    break;
			case ( DB_C_TYPE_STRING ):
	                    fprintf (stdout, "%d|%s\n", Values[i].cat, Values[i].str1);	        
			    break;
		    }
		}
	    }
            break;
    } 

    return 0;
}

int 
print_stat (void)
{
    G_message (_("%d categories read from map"), vstat.rcat);        
    G_message (_("%d records selected from table"), vstat.select);        
    G_message (_("%d categories read from map exist in selection from table"), vstat.exist);
    G_message (_("%d categories read from map don't exist in selection from table"), vstat.notexist);    
    G_message (_("%d records updated/inserted"), vstat.update);    
    G_message (_("%d update/insert errors"), vstat.error);
    if ( vstat.dupl > 0 )
	G_message (_("%d categories with more points (coordinates not loaded)"), vstat.dupl);        

    return 0;
}    
