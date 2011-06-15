#include <stdio.h>
#include <grass/gis.h>
#include <grass/G3d.h>
#include "G3d_intern.h"

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

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Converts window-coordinates <em>(north, east,
 *  top)</em> into cell-coordinates <em>(x, y, z)</em>.
 *
 *  \param map
 *  \param north
 *  \param east
 *  \param top
 *  \param x
 *  \param y
 *  \param z
 *  \return void
 */

void
G3d_location2WindowCoord(G3D_Map * map, double north, double east, double top,
		   int *x, int *y, int *z)
{
    double col, row, depth;
    
    col = (east - map->window.west) / (map->window.east -
				      map->window.west) * (double)(map->window.cols);
    row = (north - map->window.south) / (map->window.north -
					map->window.south) * (double)(map->window.rows);
    depth = (top - map->window.bottom) / (map->window.top -
				       map->window.bottom) * (double)(map->window.depths);
    /*
    printf("G3d_location2WindowCoord col %g row %g depth %g\n", col, row, depth);
    */
    
    *x = (int)col;
    *y = (int)row;
    *z = (int)depth;
    
}

/*!
 * \brief 
 *
 *  Converts window-coordinates <em>(north, east,
 *  top)</em> into cell-coordinates <em>(x, y, z)</em>.
 *  This function calls G3d_fatalError in case location is not in window.
 *
 *  \param map
 *  \param north
 *  \param east
 *  \param top
 *  \param x
 *  \param y
 *  \param z
 *  \return void
 */

void
G3d_location2WindowCoord2(G3D_Map * map, double north, double east, double top,
		   int *x, int *y, int *z)
{
    if (!G3d_isValidLocationWindow(map, north, east, top))
	G3d_fatalError("G3d_location2WindowCoord2: location not in window");
    
    G3d_location2WindowCoord(map, north, east, top, x, y, z);
}
