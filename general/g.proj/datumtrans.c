/*  
 ****************************************************************************
 *
 * MODULE:       g.proj 
 * AUTHOR(S):    Paul Kelly - paul-grass@stjohnspoint.co.uk
 * PURPOSE:      Provides a means of reporting the contents of GRASS
 *               projection information files and creating
 *               new projection information files.
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/gprojects.h>

#include "local_proto.h"

/**
 * 
 * \brief Add or replace datum transformation parameters to the current
 *        co-ordinate system definition
 * 
 * \param datumtrans  Index number of parameter set to use, 0 to leave
 *                    unspecifed (or remove specific parameters, leaving just
 *                    the datum name), -1 to list the available parameter
 *                    sets for this datum and exit.
 *
 * \param force       Force editing of parameters even if current co-ordinate
 *                    system already contains fully specified parameters.
 * 
 * \param interactive Interactively prompt user for parameters, ignoring
 *                    value of datumtrans variable.
 * 
 * \return            1 if a change was made, 0 if not.
 **/

int set_datumtrans(int datumtrans, int force, int interactive)
{
    char *params, *datum = NULL;
    int paramsets, status;
    struct gpj_datum dstruct;

    if(cellhd.proj == PROJECTION_XY)
        return 0;

    status = GPJ__get_datum_params(projinfo, &datum, &params);
    if( datum )
    {
        /* A datum name is specifed; need to determine if
	 * there are parameters to choose from for this datum */       
       
        if( GPJ_get_datum_by_name(datum, &dstruct) > 0 ) {
    	    char *defparams;
	   
            paramsets = GPJ_get_default_datum_params_by_name(dstruct.name, &defparams);
	   
	    if( status == 1 && paramsets > 1 ) 
	        /* Parameters are missing and there is a choice to be made */
	        force = 1;
	   
	}
        else
            /* Datum name not found in table; can't do anything. */
            force = 0;
	 
    }
    else
        /* No datum name; can't do anything. */
        force = 0;
   
    if( force )
    {
	char *chosenparams = NULL;
	char *paramkey, *paramvalue;	    
        struct Key_Value *temp_projinfo;
	int i;
       
        /* First of all obtain the new parameters either interactively
	 * or through the supplied transform number index */
        if( interactive )
        {
            /* Print to stderr here as the function call will also */
            fprintf(stderr, "Datum name: %s (%s)", dstruct.name, 
                    dstruct.longname);
            GPJ_ask_datum_params(datum, &chosenparams);
        }
        else
	{
            struct gpj_datum_transform_list *list;

	    if( datumtrans > paramsets )
	        G_fatal_error("Invalid tranformation number %d; valid range is 1 to %d",
			      datumtrans, paramsets);
	   
            list = GPJ_get_datum_transform_by_name( datum );
	   
	    if( list != NULL )
	    {
	        if( datumtrans == -1 ) 
		{		 		    
	            do
                    {
                        fprintf(stdout, "---\n%d\nUsed in %s\n%s\n%s\n",
        	                list->count, list->where_used, 
				list->params, list->comment);
	                list = list->next;
                    } while(list != NULL);
		   
		    exit(EXIT_SUCCESS);
		}	       
		else
		{
	            do
                    {
 		        if( list->count == datumtrans )
		        {			    
			    chosenparams = G_store( list->params );
			    break;
		        }
	                list = list->next;
                    } while(list != NULL);
		}		   
	    }
	   
	}
	   
	temp_projinfo = G_create_key_value();
	
	/* Copy old PROJ_INFO, skipping out any keys related
	 * to datum parameters */
	for (i = 0; i < projinfo->nitems; i++)
	{
	    if( strcmp(projinfo->key[i], "dx") == 0
		|| strcmp(projinfo->key[i], "dy") == 0
		|| strcmp(projinfo->key[i], "dz") == 0
		|| strcmp(projinfo->key[i], "datumparams") == 0
		|| strcmp(projinfo->key[i], "nadgrids") == 0
		|| strcmp(projinfo->key[i], "towgs84") == 0 )
		continue;
	   
	    G_set_key_value(projinfo->key[i], projinfo->value[i], temp_projinfo);
	}
	   
        /* Finally add new parameters (if we have them) */
        if( chosenparams != NULL )
	{		
	    /* Now split 'chosenparams' into key/value format */
	    paramkey = strtok(chosenparams, "=");
	    paramvalue = chosenparams + strlen(paramkey) + 1;
	    G_set_key_value( paramkey, paramvalue, temp_projinfo );
	    G_free( chosenparams );
	}	   
		   	   
	/* Destroy original key/value structure and replace with new one */
	G_free_key_value( projinfo );
	projinfo = temp_projinfo;

    }
   
    return force;
}
