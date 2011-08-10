#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

G3D_Region g3d_window;

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Sets the window for <em>map</em> to <em>window</em>.
 * Can be used multiple times for the same map.
 *
 *  \param map
 *  \param window
 *  \return void
 */

void G3d_setWindowMap(G3D_Map * map, G3D_Region * window)
{
    G3d_regionCopy(&(map->window), window);
    G3d_adjustRegion(&(map->window));
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Sets the default window used for every map opened later in the program.
 * Can be used multiple times in the same program.
 *
 *  \param window
 *  \return void
 */

void G3d_setWindow(G3D_Region * window)
{
    G3d_regionCopy(&g3d_window, window);
    G3d_adjustRegion(&g3d_window);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Stores the current default window in <em>window</em>.
 *
 *  \param window
 *  \return void
 */

void G3d_getWindow(G3D_Region * window)
{
    G3d_regionCopy(window, &g3d_window);
}

/*---------------------------------------------------------------------------*/

G3D_Region *G3d_windowPtr()
{
    return &g3d_window;
}


/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Returns 1 if window-coordinates <em>(north, west, bottom)</em> are
 * inside the window of <em>map</em>. Returns 0 otherwise.
 *
 *  \param map
 *  \param north
 *  \param east
 *  \param top
 *  \return int
 */

int G3d_isValidLocationWindow(G3D_Map * map, double north, double east, double top)
{
    return ((north >= map->window.south) && (north <= map->window.north) &&
	    (east >= map->window.west) && (east <= map->window.east) &&
	    (((top >= map->window.bottom) && (top <= map->window.top)) ||
	     ((top <= map->window.bottom) && (top >= map->window.top))));
}
