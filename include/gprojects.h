/*
 ******************************************************************************
 *
 * MODULE:       gproj library
 * AUTHOR(S):    Original Author unknown, probably Soil Conservation Service
 *               Paul Kelly
 * PURPOSE:      Include file for GRASS modules that use the PROJ.4
 *               wrapper functions
 * COPYRIGHT:    (C) 2003 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#ifndef GRASS_GPROJECTS_H
#define GRASS_GPROJECTS_H

#include <grass/config.h>
/* TODO: clean up support for PROJ 5+ */
#ifdef HAVE_PROJ_H
#include <proj.h>
#define RAD_TO_DEG    57.295779513082321
#define DEG_TO_RAD   .017453292519943296
#else
#include <proj_api.h>
#define PJ_FWD 	 1
#define PJ_INV 	-1
#endif
#ifdef HAVE_OGR
#    include <ogr_srs_api.h>
#endif

/* Data Files */
#define ELLIPSOIDTABLE "/etc/proj/ellipse.table"
#define DATUMTABLE "/etc/proj/datum.table"
#define DATUMTRANSFORMTABLE "/etc/proj/datumtransform.table"
/* GRASS relative location of datum conversion lookup tables */
#define GRIDDIR "/etc/proj/nad"

/* TODO: rename pj_ to gpj_ to avoid symbol clash with PROJ lib */
struct pj_info
{
#ifdef HAVE_PROJ_H
    PJ *pj;
#else
    projPJ pj;
#endif
    double meters;
    int zone;
    char proj[100];
    char *def;
    char *srid;
};

struct gpj_datum
{
    char *name, *longname, *ellps;
    double dx, dy, dz;
};

struct gpj_datum_transform_list
{

    int count;			/**< Transform Number (ordered list) */

    char *params;		/**< PROJ.4-style datum transform parameters */

    char *where_used;		/**< Comment text describing where (geographically)
				 * the transform is valid */

    char *comment;		/**< Additional Comments */

    struct gpj_datum_transform_list *next;	/**< Pointer to next set of 
					 * transform parameters in linked list */
};

struct gpj_ellps
{
    char *name, *longname;
    double a, es, rf;
};

#ifndef HAVE_PROJ_H
/* PROJ.4's private datastructures copied from projects.h as removed
   from upstream; pending better solution. see:
   http://trac.osgeo.org/proj/ticket/98 */

/* In PROJ 5, the 'struct FACTORS' is back in as 'struct P5_FACTORS',
 * and old 'struct LP' is now back in as 'PJ_UV' */

typedef struct { double u, v; } LP;

struct DERIVS {
    double x_l, x_p; /* derivatives of x for lambda-phi */
    double y_l, y_p; /* derivatives of y for lambda-phi */
};

struct FACTORS {
	struct DERIVS der;
	double h, k;		/* meridinal, parallel scales */
	double omega, thetap;	/* angular distortion, theta prime */
	double conv;		/* convergence */
	double s;		/* areal scale factor */
	double a, b;		/* max-min scale error */
	int code;		/* info as to analytics, see following */
};
/* end of copy */
#endif

#include <grass/defs/gprojects.h>

#endif
