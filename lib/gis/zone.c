/*!
 * \file lib/gis/zone.c
 *
 * \brief GIS Library - Cartographic zone functions.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
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
