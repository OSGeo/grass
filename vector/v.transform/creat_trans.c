/*
****************************************************************************
*
* MODULE:       v.transform
* AUTHOR(S):    See other files as well...
*               Eric G. Miller <egm2@jps.net>
* PURPOSE:      To transform a vector layer's coordinates via a set of tie
*               points.
* COPYRIGHT:    (C) 2002 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

/*
*  create_transform_conversion () - main driver routine to prepare
*    the transformation equation.
*
*  yes_no_quest(s)  -  ask the user a yes, no question.
*     returns:   1 for yes, 0 for no
*  Written by the GRASS Team, 02/16/90, -mh.
*/

#include <stdio.h>
#include <stdlib.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include "trans.h"
#include "local_proto.h"

int 
create_transform_conversion (struct file_info *Coord, int quiet)
{
	if ( Coord->name[0] != '\0')
		create_transform_from_file( Coord, quiet) ;
	else
		create_transform_from_user () ;

	return 0;
}

int 
create_transform_from_user (void)
{
	int  status ;
	int  n_points;
	int  ok ;

init_transform_arrays() ;

n_points = 0 ;
ok = 0 ;
while (! ok)
 {
#ifdef __MINGW32__       
        G_fatal_error ( "Points cannot be entered interactively on Windows" );
#else
	/*  go to Vask page to enter the coordinates  */
	if ((n_points =  ask_transform_coor (n_points)) < 0)
		exit(-1) ;
#endif

	G_clear_screen() ;

	status =  setup_transform( n_points) ;


	if (status != ALL_OK )
	{
		G_message ( _(" Number of points that have been entered: %d\n"), n_points );
		print_transform_error(status) ;
		continue ;
	}

	print_transform_resids( n_points) ;
	ok = yes_no_quest ("\n\n\nIf satisfied with the residuals, enter 'y', else 'n' and <Return>:  ");

 }		/*  while (!ok)   */


 return(0) ;

}			/*  create_transform_conversion()  */

int 
yes_no_quest (char *s)
{
    char buff[200];
    while (1)
    {
	G_message ("%s", s);
	if (NULL == fgets(buff,200,stdin))
		exit(-1) ;
	switch (*buff)
	{
	    case 'Y': case 'y':
		return(1);
	    case 'N': case 'n':
		return(0);
	    default:
		G_message ( _("Please answer yes or no"));
	}
    }
}


int 
create_transform_from_file (struct file_info *Coord, int quiet)
{
	int  status ;
	int  n_points;

	init_transform_arrays() ;

	n_points = 0 ;
/*  Get the coordinates from the file.  */
	if ((n_points =  get_coor_from_file (Coord->fp) ) < 0)
		exit(-1) ;

	status =  setup_transform( n_points) ;

	if (status != ALL_OK )
	{
		G_message ( _("Number of points that have been entered [%d]"), n_points );
		print_transform_error(status) ;
		exit(-1) ;
	}

	if (!quiet)
		print_transform_resids( n_points) ;

 return(0) ;

}			/*  create_transform_from_file()  */
