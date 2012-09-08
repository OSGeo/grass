
/**
 * \file diglib/type.c
 *
 * \brief Vector library - feature type conversion (lower level functions)
 *
 * Lower level functions for reading/writing/manipulating vectors.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 *
 * \date 2001
 */

#include <grass/vector.h>

/*!
   \brief Convert type to store type

   \param type feature type

   \return store type
 */
int dig_type_to_store(int type)
{
    switch (type) {
    case GV_POINT:
	return GV_STORE_POINT;
    case GV_LINE:
	return GV_STORE_LINE;
    case GV_BOUNDARY:
	return GV_STORE_BOUNDARY;
    case GV_CENTROID:
	return GV_STORE_CENTROID;
    case GV_AREA:
	return GV_STORE_AREA;
    case GV_FACE:
	return GV_STORE_FACE;
    case GV_KERNEL:
	return GV_STORE_KERNEL;
    case GV_VOLUME:
	return GV_STORE_VOLUME;
    default:
	return 0;
    }
}

/*!
   \brief Convert type from store type

   \param stype feature store type

   \return type
 */
int dig_type_from_store(int stype)
{
    switch (stype) {
    case GV_STORE_POINT:
	return GV_POINT;
    case GV_STORE_LINE:
	return GV_LINE;
    case GV_STORE_BOUNDARY:
	return GV_BOUNDARY;
    case GV_STORE_CENTROID:
	return GV_CENTROID;
    case GV_STORE_AREA:
	return GV_AREA;
    case GV_STORE_FACE:
	return GV_FACE;
    case GV_STORE_KERNEL:
	return GV_KERNEL;
    case GV_STORE_VOLUME:
	return GV_VOLUME;
    default:
	return 0;
    }
}
