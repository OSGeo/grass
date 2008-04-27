/*!
  \file gisdbase.c
  
  \brief GIS library - environment routines (gisdbase)
  
  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Original author CERL
*/

#include <grass/gis.h>

/*!
 * \brief Get name of top level database directory
 *
 * Returns the full UNIX path name of the directory which holds the
 * database locations. See GISDBASE for a full explanation of this
 * directory.
 *
 *  \param
 *  \return pointer to string containing the base directory
 */

char *
G_gisdbase(void)
{
    return G_getenv ("GISDBASE");
}
