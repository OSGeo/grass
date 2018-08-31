/*!
  \file lib/raster/color_init.c
  
  \brief Raster Library - Initialize Colors structure
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <grass/gis.h>
#include <grass/raster.h>

/*!
  \brief Initialize color structure
  
  The <i>colors</i> structure is initialized for subsequent calls
  to Rast_add_c_color_rule() and Rast_set_c_color().
 
  \param colors pointer to Colors structure
*/
void Rast_init_colors(struct Colors *colors)
{
    G_zero(colors, sizeof(struct Colors));
    
    colors->cmax = -1;
    colors->fixed.max = -1;
    colors->modular.max = -1;
}
