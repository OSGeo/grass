#include <stdio.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/G3d.h>

#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Returns in <em>region2d</em> the <em>2d</em> portion of <em>region3d</em>.
 *
 *  \param region3d
 *  \param region2d
 *  \return void
 */

void G3d_extract2dRegion(G3D_Region * region3d, struct Cell_head *region2d)
{
    region2d->proj = region3d->proj;
    region2d->zone = region3d->zone;

    region2d->north = region3d->north;
    region2d->south = region3d->south;
    region2d->east = region3d->east;
    region2d->west = region3d->west;

    region2d->rows = region3d->rows;
    region2d->cols = region3d->cols;

    region2d->ns_res = region3d->ns_res;
    region2d->ew_res = region3d->ew_res;
}

/*!
 * \brief 
 *
 *  Returns in <em>region2d</em> the <em>2d</em> portion of <em>region3d</em>.
 *
 *  \param region3d
 *  \param region2d
 *  \return void
 */

void G3d_regionToCellHead(G3D_Region * region3d, struct Cell_head *region2d)
{
    region2d->proj = region3d->proj;
    region2d->zone = region3d->zone;

    region2d->north = region3d->north;
    region2d->south = region3d->south;
    region2d->east = region3d->east;
    region2d->west = region3d->west;
    region2d->top = region3d->top;
    region2d->bottom = region3d->bottom;

    region2d->rows = region3d->rows;
    region2d->rows3 = region3d->rows;
    region2d->cols = region3d->cols;
    region2d->cols3 = region3d->cols;
    region2d->depths = region3d->depths;

    region2d->ns_res = region3d->ns_res;
    region2d->ns_res3 = region3d->ns_res;
    region2d->ew_res = region3d->ew_res;
    region2d->ew_res3 = region3d->ew_res;
    region2d->tb_res = region3d->tb_res;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Replaces the <em>2d</em> portion of <em>region3d</em> with the
 * values stored in <em>region2d</em>.
 *
 *  \param region2d
 *  \param region3d
 *  \return void
 */

void
G3d_incorporate2dRegion(struct Cell_head *region2d, G3D_Region * region3d)
{
    region3d->proj = region2d->proj;
    region3d->zone = region2d->zone;

    region3d->north = region2d->north;
    region3d->south = region2d->south;
    region3d->east = region2d->east;
    region3d->west = region2d->west;

    region3d->rows = region2d->rows;
    region3d->cols = region2d->cols;

    region3d->ns_res = region2d->ns_res;
    region3d->ew_res = region2d->ew_res;
}

/*!
 * \brief 
 *
 * Replaces the <em>2d</em> portion of <em>region3d</em> with the
 * values stored in <em>region2d</em>.
 *
 *  \param region2d
 *  \param region3d
 *  \return void
 */

void
G3d_regionFromToCellHead(struct Cell_head *region2d, G3D_Region * region3d)
{
    region3d->proj = region2d->proj;
    region3d->zone = region2d->zone;

    region3d->north = region2d->north;
    region3d->south = region2d->south;
    region3d->east = region2d->east;
    region3d->west = region2d->west;
    region3d->top = region2d->top;
    region3d->bottom = region2d->bottom;

    region3d->rows = region2d->rows3;
    region3d->cols = region2d->cols3;
    region3d->depths = region2d->depths;

    region3d->ns_res = region2d->ns_res3;
    region3d->ew_res = region2d->ew_res3;
    region3d->tb_res = region2d->tb_res;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Computes an adjusts the resolutions in the region structure from the region
 * boundaries and number of cells per dimension.
 *
 *  \param region
 *  \return void
 */

void G3d_adjustRegion(G3D_Region * region)
{
    struct Cell_head region2d;

    G3d_regionToCellHead(region, &region2d);
    G_adjust_Cell_head3(&region2d, 1, 1, 1);
    G3d_regionFromToCellHead(&region2d, region);

    if (region->depths <= 0)
	G3d_fatalError("G3d_adjustRegion: depths <= 0");
    region->tb_res = (region->top - region->bottom) / region->depths;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Computes an adjusts the number of cells per dimension in the region
 * structure from the region boundaries and resolutions.
 *
 *  \param region
 *  \return void
 */

void G3d_adjustRegionRes(G3D_Region * region)
{
    struct Cell_head region2d;

    G3d_regionToCellHead(region, &region2d);
    G_adjust_Cell_head3(&region2d, 1, 1, 1);
    G3d_regionFromToCellHead(&region2d, region);

    if (region->tb_res <= 0)
	G3d_fatalError("G3d_adjustRegionRes: tb_res <= 0");

    region->depths = (region->top - region->bottom + region->tb_res / 2.0) /
	region->tb_res;
    if (region->depths == 0)
	region->depths = 1;
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Copies the values of <em>regionSrc</em> into <em>regionDst</em>.
 *
 *  \param regionDest
 *  \param regionSrc
 *  \return void
 */

void G3d_regionCopy(G3D_Region * regionDest, G3D_Region * regionSrc)
{
    *regionDest = *regionSrc;
}


/*---------------------------------------------------------------------------*/

int
G3d_readRegionMap(const char *name, const char *mapset, G3D_Region * region)
{
    char fullName[GPATH_MAX];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    if (G_name_is_fully_qualified(name, xname, xmapset))
	G3d_filename(fullName, G3D_HEADER_ELEMENT, xname, xmapset);
    else {
	if (!mapset || !*mapset)
	    mapset = G_find_grid3(name, "");
	G3d_filename(fullName, G3D_HEADER_ELEMENT, name, mapset);
    }
    return G3d_readWindow(region, fullName);
}

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Returns 1 if region-coordinates <em>(north, west, bottom)</em> are
 * inside the region of <em>map</em>. Returns 0 otherwise.
 *
 *  \param map
 *  \param north
 *  \param east
 *  \param top
 *  \return int
 */

int G3d_isValidLocation(G3D_Map * map, double north, double east, double top)
{
    return ((north >= map->region.south) && (north <= map->region.north) &&
	    (east >= map->region.west) && (east <= map->region.east) &&
	    (((top >= map->region.bottom) && (top <= map->region.top)) ||
	     ((top <= map->region.bottom) && (top >= map->region.top))));
}

/*---------------------------------------------------------------------------*/

/*!
 * \brief 
 *
 *  Converts region-coordinates <em>(north, east,
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
G3d_location2coord(G3D_Map * map, double north, double east, double top,
		   int *x, int *y, int *z)
{
    double col, row, depth;
    
    col = (east - map->region.west) / (map->region.east -
				      map->region.west) * (double)(map->region.cols);
    row = (north - map->region.south) / (map->region.north -
					map->region.south) * (double)(map->region.rows);
    depth = (top - map->region.bottom) / (map->region.top -
				       map->region.bottom) * (double)(map->region.depths);
    /*
    printf("G3d_location2coord col %g row %g depth %g\n", col, row, depth);
    */  
    
    *x = (int)col;
    *y = (int)row;
    *z = (int)depth;
}


/*!
 * \brief 
 *
 *  Converts region-coordinates <em>(north, east,
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
G3d_location2coord2(G3D_Map * map, double north, double east, double top,
		   int *x, int *y, int *z)
{
    if (!G3d_isValidLocation(map, north, east, top))
	G3d_fatalError("G3d_location2coord2: location not in region");

    G3d_location2coord(map, north, east, top, x, y, z);
}
