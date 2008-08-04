
/*-
 * $Log$
 * Revision 2.1  2006-02-09 03:08:54  glynn
 * Use <grass/gis.h> etc rather than <gis.h>
 *
 * Revision 2.0  2004/11/09 13:06:32  bernhard
 * copied within CVS repository from grass/src/include/site.h
 *
 * Revision 1.7  2000/10/09 01:57:03  eric
 * Added ability to ignore z-value completely using field=SITE_COL_NUL
 *
 * Revision 1.6  2000/10/07 21:23:24  eric
 * Added Cell_head *region parameter. Now respects region! Fixed index bug for
 * dimensions.
 *
 * Revision 1.5  2000/10/06 04:13:53  eric
 * Added the G_readsites_xyz() function and related G_alloc_site_xyz() and
 * G_free_site_xyz() convenience functions.  Will send Markus a short LaTeX
 * documentation and example...
 *
 * Revision 1.4  2000/01/02 12:23:20  markus
 * again comments fixed
 *
 * Revision 1.3  2000/01/02 12:21:47  markus
 * comments fixed
 *
 * Revision 1.2  2000/01/02 12:18:16  markus
 * Bill Hughes: some variable types fixed
 *
 * Revision 1.1.1.1  1999/12/29 15:10:23  markus
 * initial CVS import
 *
 * Revision 1.13  1996/05/23  brown - brown@gomez.gis.uiuc.edu
 * changed DateTime stuff to use TimeStamp instead
 *
 * Revision 1.12  1995/07/17  11:16:26  mccauley
 * took out has_cat and made part of struct
 * support for floating point categories
 *
 * Revision 1.11  1995/06/20  10:26:48  mccauley
 * moved prototypes to external file: P_site.h
 *
 * Revision 1.10  1995/05/24  00:05:58  mccauley
 * added DateTime stuff
 *
 * Revision 1.9  1995/04/17  22:58:24  mccauley
 * added "typedef struct ... Site_head;"
 *
 * Revision 1.8  1995/02/22  02:46:20  mccauley
 * added sites functions from 4.1 that we'll keep.
 *
 * Revision 1.7  1995/02/22  02:25:28  mccauley
 * changed names of functions to G_site_xxx().
 *
 * Revision 1.6  1995/02/22  02:16:41  mccauley
 * increased MAX_SITE_LEN and MAX_SITE_STRING on suggestion
 * of Michael Shapiro <mshapiro@ncsa.uiuc.edu>.
 *
 * Revision 1.5  1995/02/21  07:56:11  mccauley
 * added pragma ident
 *
 * Revision 1.4  1995/02/21  07:28:18  mccauley
 * added qsort comparison function definitions.
 *
 * Revision 1.3  1995/02/08  23:10:46  mccauley
 * added prototype for G_guess_site_fmt.
 *
 * Revision 1.2  1995/02/07  23:18:00  mccauley
 * added prototypes for G_new_get_site and G_new_put_site
 *
 * Revision 1.1  1995/02/07  21:00:51  mccauley
 * Initial revision
 * 
 */

/*-
 * easting|northing|[z|[d4|]...][#category] [ [@attr_text OR %flt] ... ]
 *
 * to allow multidimensions (everything preceding the last '|') and any
 * number of text or numeric attribute fields.
 */

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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  *
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
