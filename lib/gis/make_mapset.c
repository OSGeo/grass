/******************************************************************************
 *
 * Project:  libgrass
 * Purpose:  Function to create a new mapset within an existing location
 * Author(s): Joel Pitt, joel.pitt@gmail.com
 *
 ******************************************************************************
 * Copyright (c) 2006, Joel Pitt
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 ******************************************************************************
 *
 */

#include <grass/gis.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/*
 * Returns 0 on success.
 * Returns -1 to indicate a system error (check errno).
 */
 

int G__make_mapset( const char *gisdbase_name, const char *location_name, const char *mapset_name )
{
    char	path[GPATH_MAX];
    struct Cell_head default_window;

    /* Get location */
    if (location_name == NULL)
        location_name = G_location();
    
    /* Get GISDBASE */
    if (gisdbase_name == NULL)
        gisdbase_name = G_gisdbase();
        
    /* TODO: Should probably check that user specified location and gisdbase are valid */
    
    /* Make the mapset. */
    sprintf( path, "%s/%s/%s", gisdbase_name, location_name, mapset_name );
    if( G_mkdir( path ) != 0 )
        return -1;

    G__create_alt_env();

    /* Get PERMANENT default window */
    G__setenv( "GISDBASE", gisdbase_name );
    G__setenv( "LOCATION", location_name );
    G__setenv( "MAPSET", "PERMANENT" );
    G_get_default_window( &default_window );

    /* Change to the new mapset */
    G__setenv( "MAPSET", mapset_name );

    /* Copy default window/regions to new mapset */
    G__put_window( &default_window, "", "WIND" );

    /* And switch back to original environment */
    G__switch_env();

    return 0;
}


/*!
 * \brief  create a new mapset
 * 
 * This function creates a new mapset in the current location,
 * initializes default window and current window.
 *
 * \param char * gisdbase_name
 *                      The full path of GISDBASE to create mapset in.
 *                      If NULL then current GISDBASE is used.
 * \param char * location_name
 *                      The name location to create mapset in.
 *                      If NULL then current location is used.
 * \param char * mapset_name
 *                      The name of the new mapset.  Should not include
 *                      the full path, the mapset will be created within
 *                      the current database and location.
 *
 * \return Returns 0 on success, or generates a fatal error on failure.  
 *         The G__make_mapset() function operates the same, but returns a
 *         non-zero error code on failure, instead of terminating. 
*/

int G_make_mapset( const char *gisdbase_name, const char *location_name, const char *mapset_name )
{
    int	err;

    err = G__make_mapset( gisdbase_name, location_name, mapset_name );

    if( err == 0 )
        return 0;

    if( err == -1 )
    {
        perror( "G_make_mapset" );
    }

    G_fatal_error( "G_make_mapset failed." );
    
    return 1;
}
