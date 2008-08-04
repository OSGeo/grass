
/**********************************************************************
 *  G_projection()
 *
 *  Returns the projection type of the currently set window.
 *  (Note this is really the coordinate system, not the projection)
 *    PROJECTION_XY  0 - x,y (Raw imagery)
 *    PROJECTION_UTM 1 - UTM   Universal Transverse Mercator
 *    PROJECTION_SP  2 - State Plane (in feet)
 *    PROJECTION_LL  3 - Latitude-Longitude
 *
 **********************************************************************/

#include <grass/gis.h>


/*!
 * \brief query cartographic projection
 *
 * This routine returns a code indicating the projection for the active region.  The current
 * values are:
 * 0 unreferenced x,y (imagery data)
 * 1 UTM
 * 2 State Plane
 * 3 Latitude-Longitude\remarks{Latitude-Longitude is not yet fully supported in
 * GRASS.}
 * Others may be added in the future. HINT GRASS 5: 121 projections!!
 *
 *  \param void
 *  \return int
 */

int G_projection(void)
{
    struct Cell_head window;

    G_get_set_window(&window);
    return window.proj;
}
