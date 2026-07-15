/******************************************************************************
 *
 * MODULE:       gproj library
 * AUTHOR(S):    Original Author unknown, probably Soil Conservation Service
 *               Paul Kelly
 * PURPOSE:      Include file for GRASS modules that use the PROJ
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

#include <ogr_srs_api.h>

/* TODO: clean up support for PROJ 5+ */
#include <proj.h>
#define RAD_TO_DEG 57.295779513082321
#define DEG_TO_RAD .017453292519943296

#ifndef PJ_WKT2_LATEST
/* update if new PROJ versions support new WKT2 standards */
#define PJ_WKT2_LATEST PJ_WKT2_2019
#endif

/* Data Files */
#define ELLIPSOIDTABLE      "/etc/proj/ellipse.table"
#define DATUMTABLE          "/etc/proj/datum.table"
#define DATUMTRANSFORMTABLE "/etc/proj/datumtransform.table"
/* GRASS relative location of datum conversion lookup tables */
#define GRIDDIR             "/etc/proj/nad"

/* TODO: rename pj_ to gpj_ to avoid symbol clash with PROJ lib */
struct pj_info {
    PJ *pj;
    double meters;
    int zone;
    char proj[100];
    char *def;
    char *srid;
    char *wkt;
};

struct gpj_datum {
    char *name, *longname, *ellps;
    double dx, dy, dz;
};

/* no longer needed with PROJ6+ */
struct gpj_datum_transform_list {

    int count; /**< Transform Number (ordered list) */

    char *params; /**< PROJ.4-style datum transform parameters */

    char *where_used; /**< Comment text describing where (geographically)
                       * the transform is valid */

    char *comment; /**< Additional Comments */

    struct gpj_datum_transform_list
        *next; /**< Pointer to next set of
                * transform parameters in linked list */
};

struct gpj_ellps {
    char *name, *longname;
    double a, es, rf;
};

#include <grass/defs/gprojects.h>

#endif
