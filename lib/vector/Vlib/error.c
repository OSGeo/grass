/*!
  \file error.c
  
  \brief Vector library - error management
  
  Higher level functions for reading/writing/manipulating vectors.

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

static int fatal_err = GV_FATAL_EXIT;

/*!
  \brief Set behaviour if fatal error occurs in some functions

  - GV_FATAL_EXIT(default): print error message and exit,
  - GV_FATAL_PRINT: print error message and return error,
  - GV_FATAL_RETURN: return error

  \param  err error type

  \return 0 on success
*/
int 
Vect_set_fatal_error (int err)
{
    fatal_err = err;
    return 0;
}

/*!
  \brief Get behaviour for fatal error

  \param 

  \return GV_FATAL_EXIT(default): print error message and exit,
  \return GV_FATAL_PRINT: print error message and return error,
  \return GV_FATAL_RETURN: return error
*/
int 
Vect_get_fatal_error (void)
{
    return (fatal_err);
}



