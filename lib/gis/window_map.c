/*!
  \file lib/gis/window_map.c
  
  \brief GIS Library - Window mapping functions.
  
  (C) 2001-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <grass/gis.h>

#include "G.h"

/*!
  \brief Adjust east longitude.
 
  This routine returns an equivalent <i>east</i> that is larger, but
  no more than 360 larger than the <i>west</i> coordinate.

  <b>Note:</b> This routine should be used only with
  latitude-longitude coordinates.
 
  \param east east coordinate
  \param west west coordinate
  
  \return east coordinate
*/
double G_adjust_east_longitude(double east, double west)
{
    while (east > west + 360.0)
	east -= 360.0;
    while (east <= west)
	east += 360.0;

    return east;
}

/*!
  \brief Returns east larger than west.
  
  If the region projection is <tt>PROJECTION_LL</tt>, then this
  routine returns an equivalent <i>east</i> that is larger, but no
  more than 360 degrees larger, than the coordinate for the western
  edge of the region. Otherwise no adjustment is made and the original
  <i>east</i> is returned.
  
  \param east east coordinate
  \param window pointer to Cell_head
  
  \return east coordinate
*/
double G_adjust_easting(double east, const struct Cell_head *window)
{
    if (window->proj == PROJECTION_LL) {
	east = G_adjust_east_longitude(east, window->west);
	if (east > window->east && east == window->west + 360)
	    east = window->west;
    }

    return east;
}

/*!
  \brief Initialize window (region).
*/
void G__init_window(void)
{
    if (G_is_initialized(&G__.window_set))
	return;
    
    G_get_window(&G__.window);

    G_initialize_done(&G__.window_set);
}

