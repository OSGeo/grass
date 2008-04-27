/*!
  \file type.c
  
  \brief Vector library - feature type
  
  Higher level functions for reading/writing/manipulating vectors.

  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Radim Blazek
  
  \date 2001
*/

#include <grass/gis.h>
#include <grass/Vect.h>

/*!
  \brief Get types from options

  \param type_opt Option structure

  \return types
  \return -1 on error
*/
int
Vect_option_to_types (struct Option *type_opt )
{
    int i = 0;
    int type = 0; 

    while (type_opt->answers[i]) {
        switch ( type_opt->answers[i][0] ) {
	    case 'p':
	        type |= GV_POINT;
	        break;
            case 'l':
                type |= GV_LINE;
                break;
            case 'b':
	        type |= GV_BOUNDARY;
                break;
            case 'c':
                type |= GV_CENTROID;
                break;
            case 'f':
                type |= GV_FACE;
                break;
            case 'k':
                type |= GV_KERNEL;
                break;
            case 'a':
                type |= GV_AREA;
                break;
            case 'v':
                type |= GV_VOLUME;
                break;
        }
        i++;
    }

    return type;
}
