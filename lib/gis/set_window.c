/*!
  \file lib/gis/set_window.c
  
  \brief GIS Library - Set window (map region)
  
  (C) 2001-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <grass/gis.h>
#include <grass/glocale.h>

#include "G.h"

/*!
  \brief Get the current working window (region)
  
  The current working window values are returned in the structure
  <i>window</i>.
  
  \param[out] window window structure to be set
*/
void G_get_set_window(struct Cell_head *window)
{
    G__init_window();
    *window = G__.window;
}

/*!
  \brief Establishes 'window' as the current working window.
  
  \param window window to become operative window
*/
void G_set_window(struct Cell_head *window)
{
    /* adjust window, check for valid window */
    G_adjust_Cell_head(window, 0, 0);
    
    /* copy the window to the current window */
    G__.window = *window;
    G__.window_set = 1;
}
