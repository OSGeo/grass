#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

RASTER3D_Region g3d_window;

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

void Rast3d_set_window_map(RASTER3D_Map * map, RASTER3D_Region * window)
{
    Rast3d_region_copy(&(map->window), window);
    Rast3d_adjust_region(&(map->window));
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

void Rast3d_set_window(RASTER3D_Region * window)
{
    Rast3d_region_copy(&g3d_window, window);
    Rast3d_adjust_region(&g3d_window);
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

void Rast3d_get_window(RASTER3D_Region * window)
{
    Rast3d_region_copy(window, &g3d_window);
}

/*---------------------------------------------------------------------------*/

RASTER3D_Region *Rast3d_window_ptr()
{
    return &g3d_window;
}


/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Returns 1 if window-coordinates <em>(north, east and top)</em> are
 * inside the window of <em>map</em>. Returns 0 otherwise.
 *
 *  \param map
 *  \param north
 *  \param east
 *  \param top
 *  \return int
 */

int Rast3d_isValidLocationWindow(RASTER3D_Map * map, double north, double east, double top)
{
    return ((north >= map->window.south) && (north <= map->window.north) &&
	    (east >= map->window.west) && (east <= map->window.east) &&
	    (((top >= map->window.bottom) && (top <= map->window.top)) ||
	     ((top <= map->window.bottom) && (top >= map->window.top))));
}
