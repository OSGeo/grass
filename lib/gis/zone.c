/*!
 * \file lib/gis/zone.c
 *
 * \brief GIS Library - Cartographic zone functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>


/*!
 * \brief Query cartographic zone.
 *
 * This routine returns the zone for the active region. The meaning
 * for the zone depends on the projection. For example, zone 18 for
 * projection type 1 would be UTM zone 18.
 *
 * \return int cartographic zone
 */

int G_zone(void)
{
    struct Cell_head window;

    G_get_set_window(&window);

    return window.zone;
}
