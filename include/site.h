/*-
 * easting|northing|[z|[d4|]...][#category] [ [@attr_text OR %flt] ... ]
 *
 * to allow multidimensions (everything preceding the last '|') and any
 * number of text or numeric attribute fields.
 */

#include <grass/raster.h>

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


/* Allocate 'num' SITE_XYZ structs. Returns NULL on failure */
SITE_XYZ *G_alloc_site_xyz(size_t);


/* Free the array of SITE_XYZ struct */
void G_free_site_xyz(SITE_XYZ *);


/* G_readsites_xyz: Reads a sites file converting to a site struct of xyz
 * values and the cat value.  The Z value can come from one of the
 * n-dimensions, a double attribute, or a string attribute converted to a
 * double with strtod().  The 'size' must not be greater than the number
 * of elements in the SITE_XYZ array, or bad things will happen. The number 
 * of records read is returned or EOF on end of file. NOTE: EOF won't be
 * returned unless no records are read and the EOF bit is set. It's safe
 * to assume that if the number of records read is less than the size of
 * the array, that there aren't any more records.
 */
int G_readsites_xyz(FILE *,	/* The FILE stream to the sites file               */
		    int,	/* Attribute type: SITE_COL_DIM, etc...            */
		    int,	/* The field index (1 based) for the attribute     */
		    int,	/* Size of the array                               */
		    struct Cell_head *,	/* Respect region if not NULL */
		    SITE_XYZ * xyz	/* The site array of size 'size'                   */
    );

#include <grass/P_site.h>
