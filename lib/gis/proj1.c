/*!
   \file lib/gis/proj1.c

   \brief GIS Library - Projection support (window related)

   SPDX-FileCopyrightText: 2001-2011 by the GRASS Development Team

   SPDX-License-Identifier: GPL-2.0-or-later.

   \author Original author CERL
 */

#include <grass/gis.h>

/*!
   \brief Query cartographic projection

   This routine returns a code indicating the projection for the active
   region. The current values are (see gis.h)

   - PROJECTION_XY      0 - x,y (Raw imagery)
   - PROJECTION_UTM     1 - UTM   Universal Transverse Mercator
   - PROJECTION_SP      2 - State Plane (in feet) - not used, removed
   - PROJECTION_LL      3 - Latitude-Longitude
   - PROJECTION_OTHER  99 - others

   Others may be added in the future.

   \return projection code (see above)
 */
int G_projection(void)
{
    struct Cell_head window;

    G_get_set_window(&window);
    return window.proj;
}
