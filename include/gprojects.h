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

#ifndef _GPROJECTS_H
#define _GPROJECTS_H

#include <grass/config.h>
#include <proj_api.h>
#ifdef HAVE_OGR
#    include <ogr_srs_api.h>
#endif

/* Data Files */
#define ELLIPSOIDTABLE "/etc/ellipse.table"
#define DATUMTABLE "/etc/datum.table"
#define DATUMTRANSFORMTABLE "/etc/datumtransform.table"
/* GRASS relative location of datum conversion lookup tables */
#define GRIDDIR "/etc/nad"

struct pj_info
{
    projPJ pj;
    double meters;
    int zone;
    char proj[100];
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

/* do_proj.c */
int pj_do_proj(double *, double *, struct pj_info *, struct pj_info *);
int pj_do_transform(int, double *, double *, double *,
		    struct pj_info *, struct pj_info *);

/* get_proj.c */
int pj_get_kv(struct pj_info *, struct Key_Value *, struct Key_Value *);
int pj_get_string(struct pj_info *, char *);
int GPJ_get_equivalent_latlong(struct pj_info *, struct pj_info *);
const char *set_proj_lib(const char *);
int pj_print_proj_params(struct pj_info *, struct pj_info *);

/* convert.c */
#ifdef HAVE_OGR
char *GPJ_grass_to_wkt(struct Key_Value *, struct Key_Value *, int, int);
OGRSpatialReferenceH GPJ_grass_to_osr(struct Key_Value *, struct Key_Value *);
int GPJ_wkt_to_grass(struct Cell_head *, struct Key_Value **,
		     struct Key_Value **, const char *, int);
int GPJ_osr_to_grass(struct Cell_head *, struct Key_Value **,
		     struct Key_Value **, OGRSpatialReferenceH, int);
const char *GPJ_set_csv_loc(const char *);
#endif

/* datum.c */
int GPJ_get_datum_by_name(const char *, struct gpj_datum *);
int GPJ_get_default_datum_params_by_name(const char *, char **);
int GPJ_get_datum_params(char **, char **);
int GPJ__get_datum_params(struct Key_Value *, char **, char **);
void GPJ_free_datum(struct gpj_datum *);
struct gpj_datum_transform_list *GPJ_get_datum_transform_by_name(const char *);
void GPJ_free_datum_transform(struct gpj_datum_transform_list *);

/* ellipse.c */
int GPJ_get_ellipsoid_by_name(const char *, struct gpj_ellps *);
int GPJ_get_ellipsoid_params(double *, double *, double *);
int GPJ__get_ellipsoid_params(struct Key_Value *,
			      double *, double *, double *);
void GPJ_free_ellps(struct gpj_ellps *);


#ifdef __MINGW32__
/* PROJ.4's private datastructures copied from projects.h as removed
   from osgeo4w; pending better solution. see:
   http://trac.osgeo.org/proj/ticket/98 */

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

int pj_factors(LP, void *, double, struct FACTORS *);
/* end of copy */
#endif

#endif
