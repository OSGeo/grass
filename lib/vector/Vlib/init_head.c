/*!
  \file init_head.c
  
  \brief Vector library - init header of vector map
  
  Higher level functions for reading/writing/manipulating vectors.

  Initialize Head structure.  To make sure that we are not writing
  out garbage to a file.

  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Original author CERL, probably Dave Gerdes or Mike Higgins.
  Update to GRASS 5.7 Radim Blazek and David D. Gray.
  
  \date 2001-2008
*/

#include <string.h>
#include <grass/gis.h>
#include <grass/Vect.h>

/*!
  \brief Initialize head structure
  
  \param Map vector map

  \return 0
*/
int 
Vect__init_head (struct Map_info *Map)
{
    char buf[64];

    Map->head.organization = NULL; 
    Vect_set_organization ( Map, "" );
    Map->head.date = NULL;
    Vect_set_date ( Map, "" );
    Map->head.your_name = NULL;
    sprintf (buf, "%s", G_whoami());
    Vect_set_person ( Map, buf );
    Map->head.map_name = NULL;
    Vect_set_map_name ( Map, "" );
    Map->head.source_date = NULL;
    sprintf (buf, "%s", G_date());
    Vect_set_map_date ( Map, buf );
    Map->head.line_3 = NULL;
    Vect_set_comment ( Map, "" );
    
    Vect_set_scale ( Map, 1 );
    Vect_set_zone ( Map, 0 );
    Vect_set_thresh ( Map, 0.0 );

    Map->plus.Spidx_built = 0;
    Map->plus.release_support = 0;
    Map->plus.update_cidx = 0;

    return 0;
}

/*!
  \brief Copy header data from one to another map

  \param from target vector map 
  \param to destination vector map

  \return 0 on success
*/
int 
Vect_copy_head_data (struct Map_info *from, struct Map_info *to)
{
    Vect_set_organization ( to, Vect_get_organization(from) );
    Vect_set_date ( to, Vect_get_date(from) );
    Vect_set_person ( to, Vect_get_person(from) );
    Vect_set_map_name ( to, Vect_get_map_name(from) );
    Vect_set_map_date ( to, Vect_get_map_date(from) );
    Vect_set_comment ( to, Vect_get_comment(from) );
    
    Vect_set_scale ( to, Vect_get_scale(from) );
    Vect_set_zone ( to, Vect_get_zone(from) );
    Vect_set_thresh ( to, Vect_get_thresh(from) );
  
    return 0;
}
