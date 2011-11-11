#ifndef GRASS_SITE_H
#define GRASS_SITE_H

#include <grass/raster.h>
#include <grass/vector.h>

#define MAX_SITE_STRING 1024
#define MAX_SITE_LEN 4096

typedef struct
{
    double east, north;
    double *dim;
    int dim_alloc;
    RASTER_MAP_TYPE cattype;
    CELL ccat;
    FCELL fcat;
    DCELL dcat;
    int str_alloc;
    char **str_att;
    int dbl_alloc;
    double *dbl_att;
} Site;

typedef struct
{
    const char *name, *desc, *form, *labels, *stime;
    struct TimeStamp *time;
} Site_head;


/* ========================================================================== *
 * G_readsites_xyz(): New implementation of the readsites() library           *
 * function limited generating an xyz array SITE_XYZ.                         *
 * ========================================================================== *
 * Copyright (c) 2000 Eric G. Miller <egm2@jps.net>                           *
 * -------------------------------------------------------------------------- *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation; either version 2 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software                *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA  *
 * -------------------------------------------------------------------------- *
 */

/* Some defines for which column type to use */
#define SITE_COL_NUL 0
#define SITE_COL_DIM 1
#define SITE_COL_DBL 2
#define SITE_COL_STR 3

/* The XYZ site struct. Note the use of a union for the cat value is
 * different than the Site struct.
 */
typedef struct
{
    double x, y, z;
    RASTER_MAP_TYPE cattype;
    union
    {
	double d;
	float f;
	int c;
    } cat;
} SITE_XYZ;


struct zstruct
{
    double x, y, z;
};
typedef struct zstruct Z;

#include <grass/defs/site.h>

#endif
