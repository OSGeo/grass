/****************************************************************************
 *
 * MODULE:       r.contour
 *
 * AUTHOR(S):    Terry Baker - CERL
 *               Andrea Aime <aaime liberto it>
 *
 * PURPOSE:      Produces a vector map of specified contours from a 
 *               raster map layer.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

/*
 * Fixes 3/2001: Andrea Aime  <aaime libero.it>
 *
 * o category support bug has been fixed: r.contour now produces also dig_cats
 *   file, so d.what.vect no more complain about missing category support.
 *   Since a contour level could be floatig point I've inserted in the label
 *   the true value, whilst category number remains its integer equivalent;
 * o a little trick (derived from GMT contour program) avoids any kind of
 *   backtrack, so running v.spag -i is no more needed (I hope): data matrix
 *   is parsed, and if a cell whose data matches exactly a contour value its
 *   value is added a small quantity (the smaller one that makes cell[i] +
 *   small != cell[i], I've taken into consideration double encoding)
 * o one can specify a minimum number of point for a contour line to be put
 *   into outout, so that small spurs, single points and so on won't be
 *   present and the output will be more clear (that's optional anyway);
 * o in my opinion there were minor memory handling problems in r.contour,
 *   I've corrected them (Head.map_name was not guaranteed to be properly          
 *   terminated, Points structures in contour function were not                    
 *   deallocated).    
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <float.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "local_proto.h"

int main ( int argc, char *argv[])
{
    struct GModule *module;
    struct Option *map;
    struct Option *levels;
    struct Option *vect;
    struct Option *min;
    struct Option *max;
    struct Option *step;
    struct Option *cut;

    int i;

    struct Cell_head Wind;
    char   *name;
    char   *mapset;
    struct Map_info Map;
    DCELL   **z_array;
    struct FPRange range; 
    int fd;
    double *lev; 
    int nlevels;
    int n_cut;

    /* Attributes */
    struct field_info *Fi;
    dbDriver *Driver;
    char buf[2000];
    dbString sql;

    /* please, remove before GRASS 7 released */
    struct Flag *q_flag;
    struct Flag *n_flag;


    G_gisinit (argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description = _("Produces a vector map layer of specified "
			    "contours from a raster map layer.");

    map = G_define_standard_option(G_OPT_R_INPUT);

    vect = G_define_standard_option(G_OPT_V_OUTPUT);

    levels=G_define_option () ;
    levels->key        = "levels";
    levels->type       = TYPE_DOUBLE;
    levels->required   = NO;
    levels->multiple   = YES;
    levels->description= _("List of contour levels") ;
    
    min=G_define_option () ;
    min->key        = "minlevel";
    min->type       = TYPE_DOUBLE;
    min->required   = NO;
    min->description= _("Minimum contour level") ;

    max=G_define_option () ;
    max->key        = "maxlevel";
    max->type       = TYPE_DOUBLE;
    max->required   = NO;
    max->description= _("Maximum contour level") ;

    step=G_define_option () ;
    step->key        = "step";
    step->type       = TYPE_DOUBLE;
    step->required   = NO;
    step->description= _("Increment between contour levels") ;

    cut=G_define_option () ;
    cut->key        = "cut";
    cut->type       = TYPE_INTEGER;
    cut->required   = NO;
    cut->answer = "0";
    cut->description= _("Minimum number of points for a contour line (0 -> no limit)") ;

    /* please, remove before GRASS 7 released */
    q_flag = G_define_flag() ;
    q_flag->key         = 'q' ;  
    q_flag->description = _("Run quietly") ;
    n_flag = G_define_flag() ;
    n_flag->key         = 'n' ;  
    n_flag->description = _("Suppress single crossing error messages") ;


    if (G_parser(argc, argv))
        exit (EXIT_FAILURE);

    /* please, remove before GRASS 7 released */
    if(q_flag->answer || n_flag->answer) {
        G_putenv("GRASS_VERBOSE","0");
        G_warning(_("The '-q' and '-n' flag is superseded and will be removed "
            "in future. Please use '--quiet' instead."));
    }

    if (!levels->answers && !step->answer) {
	G_fatal_error(_("Neither \"levels\" nor \"step\" parameter specified."));
    }

    name = map->answer;
    mapset = G_find_cell2 (name, "");
    if  (mapset == NULL)
    	G_fatal_error  (_("Raster map <%s> not found"), name);

    fd = G_open_cell_old  (name, mapset);
    if  (fd < 0)
    	G_fatal_error  (_("Unable to open raster map <%s>"), name);

    if (G_read_fp_range (name, mapset, &range) < 0)
	G_fatal_error (_("Could not read range file"));

    /* get window info */
    G_get_window  (&Wind);

    Vect_open_new (&Map, vect->answer, 1);
    Vect_hist_command (&Map);

    db_init_string (&sql);

    /* Open database, create table */
    Fi = Vect_default_field_info ( &Map, 1, NULL, GV_1TABLE );
    Vect_map_add_dblink ( &Map, Fi->number, Fi->name, Fi->table, Fi->key, Fi->database, Fi->driver);
    
    Driver = db_start_driver_open_database ( Fi->driver, Vect_subst_var(Fi->database,&Map) );
    if (Driver == NULL)
      G_fatal_error(_("Unable to open database <%s> by driver <%s>"), Fi->database, Fi->driver);

    sprintf ( buf, "create table %s ( cat integer, level double precision )", Fi->table );

    db_set_string ( &sql, buf);

    G_debug ( 1, "SQL: %s", db_get_string(&sql) );
    
    if (db_execute_immediate (Driver, &sql) != DB_OK ) {
      G_fatal_error (_("Unable to create table: %s"), db_get_string ( &sql ) );
    }

    if ( db_create_index2(Driver, Fi->table, "cat" ) != DB_OK )
	G_warning ( _("Cannot create index") );

    if (db_grant_on_table (Driver, Fi->table, DB_PRIV_SELECT, DB_GROUP|DB_PUBLIC ) != DB_OK )
	G_fatal_error ( _("Unable to grant privileges on table <%s>"), Fi->table );
    
    z_array = get_z_array (fd,Wind.rows,Wind.cols);
    lev = getlevels(levels, max, min, step, &range, &nlevels);
    displaceMatrix(z_array, Wind.rows, Wind.cols, lev, nlevels);
    n_cut = atoi(cut->answer);
    contour(lev, nlevels,  Map, z_array, Wind, n_cut);

    /* Write levels */
    for ( i = 0; i < nlevels; i++ ) {
	sprintf ( buf, "insert into %s values ( %d, %e )", Fi->table, i+1, lev[i] );
	db_set_string ( &sql, buf);

        G_debug ( 3, "SQL: %s", db_get_string(&sql) );

	if (db_execute_immediate (Driver, &sql) != DB_OK ) {
	    G_fatal_error ( _("Unable to insert row: %s"), db_get_string ( &sql ) );
	}
    }
  
    db_close_database_shutdown_driver(Driver);
    
    Vect_build (&Map, stderr);
    Vect_close (&Map);

    G_done_msg("");

    exit (EXIT_SUCCESS);
}

/*********************************************************************/
DCELL **get_z_array ( int fd, int nrow,int ncol)
{
   DCELL **z_array;
   int i;

    z_array = (DCELL **) G_malloc (nrow*sizeof(DCELL *));

    G_message (_("Reading data: "));

    for(i = 0; i < nrow; i++)
    {
	z_array[i] = (DCELL *) G_malloc (ncol * sizeof(DCELL));
        G_get_d_raster_row (fd,z_array[i],i);
        G_percent (i+1, nrow , 2);
	    
    }
   return z_array;
}


/********************************************************************/
double *getlevels(
    struct Option *levels,	
    struct Option *max,struct Option *min,
    struct Option *step,
    struct FPRange *range,
    int *num)
{
    double dmax, dmin, dstep;
    int nlevels, i, k, n;
    double j;
    DCELL zmin, zmax; /* min and max data values */
    double *lev;
    double tmp;

    G_get_fp_range_min_max(range,&zmin,&zmax);

    if(!G_is_d_null_value(&zmin) && !G_is_d_null_value(&zmax))
        G_message(_("Range of data:    min =  %f max = %f"), zmin, zmax);
    else
        G_message(_("Range of data:    empty"));

    nlevels = 0;
    if(levels->answers)
    {
	for(n = 0; levels->answers[n] != NULL; n++)
	    nlevels++;
	
	lev = (double *) G_malloc ((nlevels)*sizeof(double));

	n = nlevels;
	k = 0;
	for(i = 0; i < n; i++)
	{
	    j = atof (levels->answers[i]);
	    if ((j < zmin) || (j > zmax))
	    {
		nlevels--;
	    }
	    else
	    {
		lev[k] = j;
		k++;
	    }
	}
    }
    else /* step */
    {
		dstep = atof (step->answer);
		/* fix if step < 1, Roger Bivand 1/2001: */
		dmax = (max->answer) ? atof (max->answer) : dstep == 0 ?
	                       G_fatal_error(_("This step value is not allowed.")) : zmax - fmod(zmax, dstep);
		dmin = (min->answer) ? atof (min->answer) : dstep == 0 ?
	                       G_fatal_error(_("This step value is not allowed.")) :
	                       fmod(zmin,dstep) ? zmin - fmod(zmin,dstep) +dstep: zmin;

		while (dmin < zmin)
		{
	    	dmin += dstep;
		}
		while (dmin > zmax)
		{
		    dmin -= dstep;
		}
		
		while (dmax > zmax)
		{
	    	dmax -= dstep;
		}
		while (dmax < zmin)
		{
			dmax += dstep;
		}
		if (dmin > dmax)
		{
		    tmp = dmin;
	    	dmin = dmax;
		    dmax = tmp;
		}
		dmin = dmin < zmin ? zmin : dmin;
		dmax = dmax > zmax ? zmax : dmax;

                G_message (_("Range of levels: min = %f max = %f"), dmin, dmax);

		nlevels = (dmax - dmin)/dstep + 2;
		lev=(double *) G_malloc (nlevels * sizeof(double));
		for (nlevels = 0  ; dmin < dmax; dmin+=dstep)
		{
		    lev[nlevels] = dmin ;
		    nlevels++;
		}
		lev[nlevels] = dmax;
		nlevels++;
	}

	*num=nlevels;
	return lev;
}


/********************************************************************/
/*	parse the matrix and offset values that exactly match a			*/
/*	contour level. Contours values are added DBL_EPSILON*val, which	*/
/*	is defined in K&R as the minimum double x such as 1.0+x != 1.0	*/
/********************************************************************/
void displaceMatrix(DCELL** z, int nrow, int ncol, double* lev, int nlevels)
{
	int i, j, k;
	double *currRow;
	double currVal;
	
        G_message (_("Displacing data: "));
	
	for(i = 0; i < nrow; i++)
	{
            currRow = z[i];
            for(j = 0; j < ncol; j++)
            {
                    currVal = currRow[j];
                    for(k = 0; k < nlevels; k++)
                            if(currVal == lev[k])
                            {
                                    currRow[j] = currVal + currVal * DBL_EPSILON;
                                    break;
                            }
            }
            G_percent (i+1, nrow , 2);
	}
}

