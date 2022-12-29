
/**
   \file do_proj.c

   \brief GProj library - Functions for re-projecting point data

   \author Original Author unknown, probably Soil Conservation Service
   Eric Miller, Paul Kelly, Markus Metz

   (C) 2003-2008,2018 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.
**/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

/* a couple defines to simplify reading the function */
#define MULTIPLY_LOOP(x,y,c,m) \
do {\
   for (i = 0; i < c; ++i) {\
       x[i] *= m; \
       y[i] *= m; \
   }\
} while (0)

#define DIVIDE_LOOP(x,y,c,m) \
do {\
   for (i = 0; i < c; ++i) {\
       x[i] /= m; \
       y[i] /= m; \
   }\
} while (0)

static double METERS_in = 1.0, METERS_out = 1.0;

#ifdef HAVE_PROJ_H
#if PROJ_VERSION_MAJOR >= 6
int get_pj_area(const struct pj_info *iproj, double *xmin, double *xmax,
                double *ymin, double *ymax)
{
    struct Cell_head window;

    /* modules must set the current window, do not unset this window here */
    /* G_unset_window(); */
    G_get_set_window(&window);
    *xmin = window.west;
    *xmax = window.east;
    *ymin = window.south;
    *ymax = window.north;

    if (window.proj != PROJECTION_LL) {
	/* transform to ll equivalent */
	double estep, nstep;
	double x[85], y[85];
	int i;
	const char *projstr = NULL;
	char *indef = NULL;
	struct pj_info oproj, tproj;	/* proj parameters  */

	oproj.pj = NULL;
	oproj.proj[0] = '\0';
	tproj.def = NULL;

	if (proj_get_type(iproj->pj) == PJ_TYPE_BOUND_CRS) {
	    PJ *source_crs;

	    source_crs = proj_get_source_crs(NULL, iproj->pj);
	    if (source_crs) {
		projstr = proj_as_proj_string(NULL, source_crs, PJ_PROJ_5, NULL);
		if (projstr) {
		    indef = G_store(projstr);
		}
		proj_destroy(source_crs);
	    }
	}
	else {
	    projstr = proj_as_proj_string(NULL, iproj->pj, PJ_PROJ_5, NULL);
	    if (projstr) {
		indef = G_store(projstr);
	    }
	}

	if (indef == NULL)
	    indef = G_store(iproj->def);

	G_asprintf(&tproj.def, "+proj=pipeline +step +inv %s",
		   indef);
	G_debug(1, "get_pj_area() tproj.def: %s", tproj.def);
	tproj.pj = proj_create(PJ_DEFAULT_CTX, tproj.def);

	if (tproj.pj == NULL) {
	    G_warning(_("proj_create() failed for '%s'"), tproj.def);
	    G_free(indef);
	    G_free(tproj.def);
	    proj_destroy(tproj.pj);

	    return 0;
	}
	projstr = proj_as_proj_string(NULL, tproj.pj, PJ_PROJ_5, NULL);
	if (projstr == NULL) {
	    G_warning(_("proj_create() failed for '%s'"), tproj.def);
	    G_free(indef);
	    G_free(tproj.def);
	    proj_destroy(tproj.pj);

	    return 0;
	}
	else {
	    G_debug(1, "proj_create() projstr '%s'", projstr);
	}
	G_free(indef);

	estep = (window.west + window.east) / 21.;
	nstep = (window.north + window.south) / 21.;
	for (i = 0; i < 20; i++) {
	    x[i] = window.west + estep * (i + 1);
	    y[i] = window.north;

	    x[i + 20] = window.west + estep * (i + 1);
	    y[i + 20] = window.south;

	    x[i + 40] = window.west;
	    y[i + 40] = window.south + nstep * (i + 1);

	    x[i + 60] = window.east;
	    y[i + 60] = window.south + nstep * (i + 1);
	}
	x[80] = window.west;
	y[80] = window.north;
	x[81] = window.west;
	y[81] = window.south;
	x[82] = window.east;
	y[82] = window.north;
	x[83] = window.east;
	y[83] = window.south;
	x[84] = (window.west + window.east) / 2.;
	y[84] = (window.north + window.south) / 2.;

	GPJ_transform_array(iproj, &oproj, &tproj, PJ_FWD, x, y, NULL, 85);

	proj_destroy(tproj.pj);
	G_free(tproj.def);

	*xmin = *xmax = x[84];
	*ymin = *ymax = y[84];
	for (i = 0; i < 84; i++) {
	    if (*xmin > x[i])
		*xmin = x[i];
	    if (*xmax < x[i])
		*xmax = x[i];
	    if (*ymin > y[i])
		*ymin = y[i];
	    if (*ymax < y[i])
		*ymax = y[i];
	}

	G_debug(1, "input window north: %.8f", window.north);
	G_debug(1, "input window south: %.8f", window.south);
	G_debug(1, "input window east: %.8f", window.east);
	G_debug(1, "input window west: %.8f", window.west);

	G_debug(1, "transformed xmin: %.8f", *xmin);
	G_debug(1, "transformed xmax: %.8f", *xmax);
	G_debug(1, "transformed ymin: %.8f", *ymin);
	G_debug(1, "transformed ymax: %.8f", *ymax);
    }
    G_debug(1, "get_pj_area(): xmin %g, xmax %g, ymin %g, ymax %g",
            *xmin, *xmax, *ymin, *ymax);

    return 1;
}

char *get_pj_type_string(PJ *pj)
{
    char *pj_type = NULL;

    switch (proj_get_type(pj)) {
    case PJ_TYPE_UNKNOWN:
	G_asprintf(&pj_type, "unknown");
	break;
    case PJ_TYPE_ELLIPSOID:
	G_asprintf(&pj_type, "ellipsoid");
	break;
    case PJ_TYPE_PRIME_MERIDIAN:
	G_asprintf(&pj_type, "prime meridian");
	break;
    case PJ_TYPE_GEODETIC_REFERENCE_FRAME:
	G_asprintf(&pj_type, "geodetic reference frame");
	break;
    case PJ_TYPE_DYNAMIC_GEODETIC_REFERENCE_FRAME:
	G_asprintf(&pj_type, "dynamic geodetic reference frame");
	break;
    case PJ_TYPE_VERTICAL_REFERENCE_FRAME:
	G_asprintf(&pj_type, "vertical reference frame");
	break;
    case PJ_TYPE_DYNAMIC_VERTICAL_REFERENCE_FRAME:
	G_asprintf(&pj_type, "dynamic vertical reference frame");
	break;
    case PJ_TYPE_DATUM_ENSEMBLE:
	G_asprintf(&pj_type, "datum ensemble");
	break;
    /** Abstract type, not returned by proj_get_type() */
    case PJ_TYPE_CRS:
	G_asprintf(&pj_type, "crs");
	break;
    case PJ_TYPE_GEODETIC_CRS:
	G_asprintf(&pj_type, "geodetic crs");
	break;
    case PJ_TYPE_GEOCENTRIC_CRS:
	G_asprintf(&pj_type, "geocentric crs");
	break;
    /** proj_get_type() will never return that type, but
     * PJ_TYPE_GEOGRAPHIC_2D_CRS or PJ_TYPE_GEOGRAPHIC_3D_CRS. */
    case PJ_TYPE_GEOGRAPHIC_CRS:
	G_asprintf(&pj_type, "geographic crs");
	break;
    case PJ_TYPE_GEOGRAPHIC_2D_CRS:
    	G_asprintf(&pj_type, "geographic 2D crs");
	break;
    case PJ_TYPE_GEOGRAPHIC_3D_CRS:
	G_asprintf(&pj_type, "geographic 3D crs");
	break;
    case PJ_TYPE_VERTICAL_CRS:
	G_asprintf(&pj_type, "vertical crs");
	break;
    case PJ_TYPE_PROJECTED_CRS:
	G_asprintf(&pj_type, "projected crs");
	break;
    case PJ_TYPE_COMPOUND_CRS:
	G_asprintf(&pj_type, "compound crs");
	break;
    case PJ_TYPE_TEMPORAL_CRS:
	G_asprintf(&pj_type, "temporal crs");
	break;
    case PJ_TYPE_ENGINEERING_CRS:
	G_asprintf(&pj_type, "engineering crs");
	break;
    case PJ_TYPE_BOUND_CRS:
	G_asprintf(&pj_type, "bound crs");
	break;
    case PJ_TYPE_OTHER_CRS:
	G_asprintf(&pj_type, "other crs");
	break;
    case PJ_TYPE_CONVERSION:
	G_asprintf(&pj_type, "conversion");
	break;
    case PJ_TYPE_TRANSFORMATION:
	G_asprintf(&pj_type, "transformation");
	break;
    case PJ_TYPE_CONCATENATED_OPERATION:
	G_asprintf(&pj_type, "concatenated operation");
	break;
    case PJ_TYPE_OTHER_COORDINATE_OPERATION:
	G_asprintf(&pj_type, "other coordinate operation");
	break;
    default:
	G_asprintf(&pj_type, "unknown");
	break;
    }

    return pj_type;
}


PJ *get_pj_object(const struct pj_info *in_gpj, char **in_defstr)
{
    PJ *in_pj = NULL;

    *in_defstr = NULL;

    /* 1. SRID, 2. WKT, 3. standard pj from pj_get_kv */
    if (in_gpj->srid) {
	G_debug(1, "Trying SRID '%s' ...", in_gpj->srid);
	in_pj = proj_create(PJ_DEFAULT_CTX, in_gpj->srid);
	if (!in_pj) {
	    G_warning(_("Unrecognized SRID '%s'"), in_gpj->srid);
	}
	else {
	    *in_defstr = G_store(in_gpj->srid);
	    /* PROJ will do the unit conversion if set up from srid
	     * -> disable unit conversion for GPJ_transform */
	    /* ugly hack */
	    ((struct pj_info *)in_gpj)->meters = 1;
	}
    }
    if (!in_pj && in_gpj->wkt) {
	G_debug(1, "Trying WKT '%s' ...", in_gpj->wkt);
	in_pj = proj_create(PJ_DEFAULT_CTX, in_gpj->wkt);
	if (!in_pj) {
	    G_warning(_("Unrecognized WKT '%s'"), in_gpj->wkt);
	}
	else {
	    *in_defstr = G_store(in_gpj->wkt);
	    /* PROJ will do the unit conversion if set up from wkt
	     * -> disable unit conversion for GPJ_transform */
	    /* ugly hack */
	    ((struct pj_info *)in_gpj)->meters = 1;
	}
    }
    if (!in_pj && in_gpj->pj) {
	in_pj = proj_clone(PJ_DEFAULT_CTX, in_gpj->pj);
	*in_defstr = G_store(proj_as_wkt(NULL, in_pj, PJ_WKT2_LATEST, NULL));
	if (*in_defstr && !**in_defstr)
	    *in_defstr = NULL;
    }

    if (!in_pj) {
	G_warning(_("Unable to create PROJ object"));

	return NULL;
    }

    /* Even Rouault:
     * if info_in->def contains a +towgs84/+nadgrids clause,
     * this pipeline would apply it, whereas you probably only want
     * the reverse projection, and no datum shift.
     * The easiest would probably to mess up with the PROJ string.
     * Otherwise with the PROJ API, you could
     * instanciate a PJ object from the string,
     * check if it is a BoundCRS with proj_get_source_crs(),
     * and in that case, take the source CRS with proj_get_source_crs(),
     * and do the inverse transform on it */

    if (proj_get_type(in_pj) == PJ_TYPE_BOUND_CRS) {
	PJ *source_crs;

	G_debug(1, "found bound crs");
	source_crs = proj_get_source_crs(NULL, in_pj);
	if (source_crs) {
	    *in_defstr = G_store(proj_as_wkt(NULL, source_crs, PJ_WKT2_LATEST, NULL));
	    if (*in_defstr && !**in_defstr)
		*in_defstr = NULL;
	    in_pj = source_crs;
	}
    }

    return in_pj;
}
#endif
#endif

/**
 * \brief Create a PROJ transformation object to transform coordinates
 *        from an input SRS to an output SRS
 *
 * After the transformation has been initialized with this function,
 * coordinates can be transformed from input SRS to output SRS with
 * GPJ_transform() and direction = PJ_FWD, and back from output SRS to
 * input SRS with direction = OJ_INV.
 * If coordinates should be transformed between the input SRS and its
 * latlong equivalent, an uninitialized info_out with
 * info_out->pj = NULL can be passed to the function. In this case,
 * coordinates will be transformed between the input SRS and its
 * latlong equivalent, and for PROJ 5+, the transformation object is
 * created accordingly, while for PROJ 4, the output SRS is created as
 * latlong equivalent of the input SRS
 *
* PROJ 5+:
 *   info_in->pj must not be null
 *   if info_out->pj is null, assume info_out to be the ll equivalent
 *   of info_in
 *   create info_trans as conversion from info_in to its ll equivalent
 *   NOTE: this is the inverse of the logic of PROJ 5 which by default
 *         converts from ll to a given SRS, not from a given SRS to ll
 *         thus PROJ 5+ itself uses an inverse transformation in the
 *         first step of the pipeline for proj_create_crs_to_crs()
 *   if info_trans->def is not NULL, this pipeline definition will be
 *   used to create a transformation object
 * PROJ 4:
 *   info_in->pj must not be null
 *   if info_out->pj is null, create info_out as ll equivalent
 *   else do nothing, info_trans is not used
 *
 * \param info_in pointer to pj_info struct for input co-ordinate system
 * \param info_out pointer to pj_info struct for output co-ordinate system
 * \param info_trans pointer to pj_info struct for a transformation object (PROJ 5+)
 *
 * \return 1 on success, -1 on failure
 **/
int GPJ_init_transform(const struct pj_info *info_in,
                       const struct pj_info *info_out,
		       struct pj_info *info_trans)
{
    if (info_in->pj == NULL)
	G_fatal_error(_("Input coordinate system is NULL"));

    if (info_in->def == NULL)
	G_fatal_error(_("Input coordinate system definition is NULL"));

#ifdef HAVE_PROJ_H
#if PROJ_VERSION_MAJOR >= 6

    /* PROJ6+: enforce axis order easting, northing
     * +axis=enu (works with proj-4.8+) */

    info_trans->pj = NULL;
    info_trans->meters = 1.;
    info_trans->zone = 0;
    sprintf(info_trans->proj, "pipeline");

    /* user-provided pipeline */
    if (info_trans->def) {
	const char *projstr;

	/* info_in->pj, info_in->proj, info_out->pj, info_out->proj
	 * must be set */
	if (!info_in->pj || !info_in->proj[0] ||
	    !info_out->pj ||info_out->proj[0]) {
	    G_warning(_("A custom pipeline requires input and output projection info"));

	    return -1;
	}


	/* create a pj from user-defined transformation pipeline */
	info_trans->pj = proj_create(PJ_DEFAULT_CTX, info_trans->def);
	if (info_trans->pj == NULL) {
	    G_warning(_("proj_create() failed for '%s'"), info_trans->def);

	    return -1;
	}
	projstr = proj_as_proj_string(NULL, info_trans->pj, PJ_PROJ_5, NULL);
	if (projstr == NULL) {
	    G_warning(_("proj_create() failed for '%s'"), info_trans->def);

	    return -1;
	}
	else {
	    /* make sure axis order is easting, northing
	     * proj_normalize_for_visualization() does not work here
	     * because source and target CRS are unknown to PROJ
	     * remove any "+step +proj=axisswap +order=2,1" ?
	     *  */
	    info_trans->def = G_store(projstr);

	    if (strstr(info_trans->def, "axisswap")) {
		G_warning(_("The transformation pipeline contains an '%s' step. "
			    "Remove this step if easting and northing are swapped in the output."),
			    "axisswap");
	    }

	    G_debug(1, "proj_create() pipeline: %s", info_trans->def);

	    /* the user-provided PROJ pipeline is supposed to do
	     * all the needed unit conversions */
	    /* ugly hack */
	    ((struct pj_info *)info_in)->meters = 1;
	    ((struct pj_info *)info_out)->meters = 1;
	}
    }
    /* if no output CRS is defined,
     * assume info_out to be ll equivalent of info_in */
    else if (info_out->pj == NULL) {
	const char *projstr = NULL;
	char *indef = NULL;

	/* get PROJ-style definition */
	indef = G_store(info_in->def);
	G_debug(1, "ll equivalent definition: %s", indef);

	/* what about axis order?
	 * is it always enu?
	 * probably yes, as long as there is no +proj=axisswap step */
	G_asprintf(&(info_trans->def), "+proj=pipeline +step +inv %s",
		   indef);
	info_trans->pj = proj_create(PJ_DEFAULT_CTX, info_trans->def);
	if (info_trans->pj == NULL) {
	    G_warning(_("proj_create() failed for '%s'"), info_trans->def);
	    G_free(indef);

	    return -1;
	}
	projstr = proj_as_proj_string(NULL, info_trans->pj, PJ_PROJ_5, NULL);
	if (projstr == NULL) {
	    G_warning(_("proj_create() failed for '%s'"), info_trans->def);
	    G_free(indef);

	    return -1;
	}
	G_free(indef);
    }
    /* input and output CRS are available */
    else if (info_in->def && info_out->pj && info_out->def) {
	char *indef = NULL, *outdef = NULL;
	char *insrid = NULL, *outsrid = NULL;
	PJ *in_pj, *out_pj;
	PJ_OBJ_LIST *op_list;
	PJ_OPERATION_FACTORY_CONTEXT *operation_ctx;
	PJ_AREA *pj_area = NULL;
	double xmin, xmax, ymin, ymax;
	int op_count = 0, op_count_area = 0;

	/* get pj_area */
	/* do it here because get_pj_area() will use 
	 * the PROJ definition for simple transformation to the 
	 * ll equivalent and we need to do unit conversion */ 
	if (get_pj_area(info_in, &xmin, &xmax, &ymin, &ymax)) {
	    pj_area = proj_area_create();
	    proj_area_set_bbox(pj_area, xmin, ymin, xmax, ymax);
	}

	G_debug(1, "source proj string: %s", info_in->def);
	G_debug(1, "source type: %s", get_pj_type_string(info_in->pj));

	/* PROJ6+: EPSG must be uppercase EPSG */
	if (info_in->srid) {
	    if (strncmp(info_in->srid, "epsg", 4) == 0) {
		insrid = G_store_upper(info_in->srid);
		G_free(info_in->srid);
		((struct pj_info *)info_in)->srid = insrid;
		insrid = NULL;
	    }
	}

	in_pj = get_pj_object(info_in, &indef);
	if (in_pj == NULL || indef == NULL) {
	    G_warning(_("Input CRS not available for '%s'"), info_in->def);

	    return -1;
	}
	G_debug(1, "Input CRS definition: %s", indef);

	G_debug(1, "target proj string: %s", info_out->def);
	G_debug(1, "target type: %s", get_pj_type_string(info_out->pj));

	/* PROJ6+: EPSG must be uppercase EPSG */
	if (info_out->srid) {
	    if (strncmp(info_out->srid, "epsg", 4) == 0) {
		outsrid = G_store_upper(info_out->srid);
		G_free(info_out->srid);
		((struct pj_info *)info_out)->srid = outsrid;
		outsrid = NULL;
	    }
	}

	out_pj = get_pj_object(info_out, &outdef);
	if (out_pj == NULL || outdef == NULL) {
	    G_warning(_("Output CRS not available for '%s'"), info_out->def);

	    return -1;
	}
	G_debug(1, "Output CRS definition: %s", outdef);

	/* check number of operations */

	operation_ctx = proj_create_operation_factory_context(PJ_DEFAULT_CTX, NULL);
	/* proj_create_operations() works only if both source_crs
	 * and target_crs are found in the proj db
	 * if any is not found, proj can not get a list of operations
	 * and we have to take care of datumshift manually */
	/* list all operations irrespecitve of area and
	 * grid availability */
	op_list = proj_create_operations(PJ_DEFAULT_CTX,
					 in_pj,
					 out_pj,
					 operation_ctx);
	proj_operation_factory_context_destroy(operation_ctx);

	op_count = 0;
	if (op_list)
	    op_count = proj_list_get_count(op_list);
	if (op_count > 1) {
	    int i;

	    G_important_message(_("Found %d possible transformations"), op_count);
	    for (i = 0; i < op_count; i++) {
		const char *str;
		const char *area_of_use, *projstr;
		double e, w, s, n;
		PJ_PROJ_INFO pj_info;
		PJ *op, *op_norm;

		op = proj_list_get(PJ_DEFAULT_CTX, op_list, i);
		op_norm = proj_normalize_for_visualization(PJ_DEFAULT_CTX, op);

		if (!op_norm) {
		    G_warning(_("proj_normalize_for_visualization() failed for operation %d"),
			      i + 1);
		}
		else {
		    proj_destroy(op);
		    op = op_norm;
		}

		pj_info = proj_pj_info(op);
		proj_get_area_of_use(NULL, op, &w, &s, &e, &n, &area_of_use);
		G_important_message("************************");
		G_important_message(_("Operation %d:"), i + 1);
		if (pj_info.description) {
		    G_important_message(_("Description: %s"),
					pj_info.description);
		}
		if (area_of_use) {
		    G_important_message(" ");
		    G_important_message(_("Area of use: %s"),
					area_of_use);
		}
		if (pj_info.accuracy > 0) {
		    G_important_message(" ");
		    G_important_message(_("Accuracy within area of use: %g m"),
					pj_info.accuracy);
		}
#if PROJ_VERSION_NUM >= 6020000
		str = proj_get_remarks(op);
		if (str && *str) {
		    G_important_message(" ");
		    G_important_message(_("Remarks: %s"), str);
		}
		str = proj_get_scope(op);
		if (str && *str) {
		    G_important_message(" ");
		    G_important_message(_("Scope: %s"), str);
		}
#endif

		projstr = proj_as_proj_string(NULL, op,
				  PJ_PROJ_5, NULL);
		if (projstr) {
		    G_important_message(" ");
		    G_important_message(_("PROJ string:"));
		    G_important_message("%s", projstr);
		}
		proj_destroy(op);
	    }
	    G_important_message("************************");

	    G_important_message(_("See also output of:"));
	    G_important_message("projinfo -o PROJ -s \"%s\" -t \"%s\"",
				indef, outdef);
	    G_important_message(_("Please provide the appropriate PROJ string with the %s option"),
				"pipeline");
	    G_important_message("************************");
	}

	if (op_list)
	    proj_list_destroy(op_list);

	/* follwing code copied from proj_create_crs_to_crs_from_pj()
	 * in proj src/4D_api.cpp
	 * but using PROJ_SPATIAL_CRITERION_STRICT_CONTAINMENT */


	/* now use the current region as area of interest */
	operation_ctx = proj_create_operation_factory_context(PJ_DEFAULT_CTX, NULL);
	proj_operation_factory_context_set_area_of_interest(PJ_DEFAULT_CTX,
							    operation_ctx,
							    xmin, ymin,
							    xmax, ymax);
	proj_operation_factory_context_set_spatial_criterion(PJ_DEFAULT_CTX,
							     operation_ctx,
							     PROJ_SPATIAL_CRITERION_STRICT_CONTAINMENT);
	proj_operation_factory_context_set_grid_availability_use(PJ_DEFAULT_CTX,
								 operation_ctx,
								 PROJ_GRID_AVAILABILITY_DISCARD_OPERATION_IF_MISSING_GRID);
	/* The operations are sorted with the most relevant ones first:
	 * by descending area (intersection of the transformation area 
	 * with the area of interest, or intersection of the 
	 * transformation with the area of use of the CRS),
	 * and by increasing accuracy.
	 * Operations with unknown accuracy are sorted last,
	 * whatever their area.
	 */
	op_list = proj_create_operations(PJ_DEFAULT_CTX,
					 in_pj,
					 out_pj,
					 operation_ctx);
	proj_operation_factory_context_destroy(operation_ctx);
	op_count_area = 0;
	if (op_list)
	    op_count_area = proj_list_get_count(op_list);
	if (op_count_area == 0) {
	    /* no operations */
	    info_trans->pj = NULL;
	}
	else if (op_count_area == 1) {
	    info_trans->pj = proj_list_get(PJ_DEFAULT_CTX, op_list, 0);
	}
	else {  /* op_count_area > 1 */
	    /* can't use pj_create_prepared_operations()
	     * this is a PROJ-internal function
	     * trust the sorting of PROJ and use the first one */
	    info_trans->pj = proj_list_get(PJ_DEFAULT_CTX, op_list, 0);
	}
	if (op_list)
	    proj_list_destroy(op_list);

	/* try proj_create_crs_to_crs() */
	G_debug(1, "trying %s to %s", indef, outdef);

	/* proj_create_crs_to_crs() does not work because it calls
	 * proj_create_crs_to_crs_from_pj() which calls
	 * proj_operation_factory_context_set_spatial_criterion()
	 * with PROJ_SPATIAL_CRITERION_PARTIAL_INTERSECTION
	 * instead of
	 * PROJ_SPATIAL_CRITERION_STRICT_CONTAINMENT 
	 * 
	 * fixed in PROJ master, probably available with PROJ 7.3.x */

	/*
	info_trans->pj = proj_create_crs_to_crs(PJ_DEFAULT_CTX,
						indef,
						outdef,
						pj_area);
	*/

	if (in_pj)
	    proj_destroy(in_pj);
	if (out_pj)
	    proj_destroy(out_pj);

	if (info_trans->pj) {
	    const char *projstr;
	    PJ *pj_norm = NULL;

	    G_debug(1, "proj_create_crs_to_crs() succeeded with PROJ%d",
	            PROJ_VERSION_MAJOR);

	    projstr = proj_as_proj_string(NULL, info_trans->pj,
					    PJ_PROJ_5, NULL);

	    info_trans->def = G_store(projstr);

	    if (projstr) {
		/* make sure axis order is easting, northing
		 * proj_normalize_for_visualization() requires
		 * source and target CRS
		 * -> does not work with ll equivalent of input:
		 * no target CRS in +proj=pipeline +step +inv %s */
		pj_norm = proj_normalize_for_visualization(PJ_DEFAULT_CTX, info_trans->pj);

		if (!pj_norm) {
		    G_warning(_("proj_normalize_for_visualization() failed for '%s'"),
			      info_trans->def);
		}
		else {
		    projstr = proj_as_proj_string(NULL, pj_norm,
						  PJ_PROJ_5, NULL);
		    if (projstr && *projstr) {
			proj_destroy(info_trans->pj);
			info_trans->pj = pj_norm;
			info_trans->def = G_store(projstr);
		    }
		    else {
			proj_destroy(pj_norm);
			G_warning(_("No PROJ definition for normalized version of '%s'"),
			      info_trans->def);
		    }
		}
		G_important_message(_("Selected PROJ pipeline:"));
		G_important_message(_("%s"), info_trans->def);
		G_important_message("************************");
	    }
	    else {
		proj_destroy(info_trans->pj);
		info_trans->pj = NULL;
	    }
	}

	if (pj_area)
	    proj_area_destroy(pj_area);

	if (insrid)
	    G_free(insrid);
	if (outsrid)
	    G_free(outsrid);
	G_free(indef);
	G_free(outdef);
    }
    if (info_trans->pj == NULL) {
	G_warning(_("proj_create() failed for '%s'"), info_trans->def);

	return -1;
    }
#else /* PROJ 5 */
    info_trans->pj = NULL;
    info_trans->meters = 1.;
    info_trans->zone = 0;
    sprintf(info_trans->proj, "pipeline");

    /* user-provided pipeline */
    if (info_trans->def) {
	/* create a pj from user-defined transformation pipeline */
	info_trans->pj = proj_create(PJ_DEFAULT_CTX, info_trans->def);
	if (info_trans->pj == NULL) {
	    G_warning(_("proj_create() failed for '%s'"), info_trans->def);

	    return -1;
	}
    }
    /* if no output CRS is defined,
     * assume info_out to be ll equivalent of info_in */
    else if (info_out->pj == NULL) {
	G_asprintf(&(info_trans->def), "+proj=pipeline +step +inv %s",
		   info_in->def);
	info_trans->pj = proj_create(PJ_DEFAULT_CTX, info_trans->def);
	if (info_trans->pj == NULL) {
	    G_warning(_("proj_create() failed for '%s'"), info_trans->def);

	    return -1;
	}
    }
    else if (info_in->def && info_out->pj && info_out->def) {
	char *indef = NULL, *outdef = NULL;
	char *insrid = NULL, *outsrid = NULL;

	/* PROJ5: EPSG must be lowercase epsg */
	if (info_in->srid) {
	    if (strncmp(info_in->srid, "EPSG", 4) == 0)
		insrid = G_store_lower(info_in->srid);
	    else
		insrid = G_store(info_in->srid);
	}

	if (info_out->pj && info_out->srid) {
	    if (strncmp(info_out->srid, "EPSG", 4) == 0)
		outsrid = G_store_lower(info_out->srid);
	    else
		outsrid = G_store(info_out->srid);
	}

	info_trans->pj = NULL;

	if (insrid && outsrid) {
	    G_asprintf(&indef, "%s", insrid);
	    G_asprintf(&outdef, "%s", outsrid);

	    G_asprintf(&(info_trans->def), "+proj=pipeline +step +inv +init=%s +step +init=%s",
		       indef, outdef);

	    /* try proj_create_crs_to_crs() */
	    info_trans->pj = proj_create_crs_to_crs(PJ_DEFAULT_CTX,
						    indef,
						    outdef,
						    NULL);
	}

	if (info_trans->pj) {
	    G_debug(1, "proj_create_crs_to_crs() succeeded with PROJ5");
	}
	else {
	    if (indef) {
		G_free(indef);
		indef = NULL;
	    }
	    if (insrid) {
		G_asprintf(&indef, "+init=%s", insrid);
	    }
	    else {
		G_asprintf(&indef, "%s", info_in->def);
	    }

	    if (outdef) {
		G_free(outdef);
		outdef = NULL;
	    }
	    if (outsrid) {
		G_asprintf(&outdef, "+init=%s", outsrid);
	    }
	    else {
		G_asprintf(&outdef, "%s", info_out->def);
	    }

	    /* try proj_create() with +proj=pipeline +step +inv %s +step %s" */
	    G_asprintf(&(info_trans->def), "+proj=pipeline +step +inv %s +step %s",
		       indef, outdef);

	    info_trans->pj = proj_create(PJ_DEFAULT_CTX, info_trans->def);
	}
	if (insrid)
	    G_free(insrid);
	if (outsrid)
	    G_free(outsrid);
	G_free(indef);
	G_free(outdef);
    }
    if (info_trans->pj == NULL) {
	G_warning(_("proj_create() failed for '%s'"), info_trans->def);

	return -1;
    }

#endif
#else /* PROJ 4 */
    if (info_out->pj == NULL) {
	if (GPJ_get_equivalent_latlong(info_out, info_in) < 0) {
	    G_warning(_("Unable to create latlong equivalent for '%s'"),
	              info_in->def);

	    return -1;
	}
    }
#endif

    return 1;
}

/* TODO: rename pj_ to GPJ_ to avoid symbol clash with PROJ lib */

/**
 * \brief Re-project a point between two co-ordinate systems using a
 *        transformation object prepared with GPJ_prepare_pj()
 *
 * This function takes pointers to three pj_info structures as arguments,
 * and projects a point between the input and output co-ordinate system.
 * The pj_info structure info_trans must have been initialized with
 * GPJ_init_transform().
 * The direction determines if a point is projected from input CRS to
 * output CRS (PJ_FWD) or from output CRS to input CRS (PJ_INV).
 * The easting, northing, and height of the point are contained in the
 * pointers passed to the function; these will be overwritten by the
 * coordinates of the transformed point.
 *
 * \param info_in pointer to pj_info struct for input co-ordinate system
 * \param info_out pointer to pj_info struct for output co-ordinate system
 * \param info_trans pointer to pj_info struct for a transformation object (PROJ 5+)
 * \param dir direction of the transformation (PJ_FWD or PJ_INV)
 * \param x Pointer to a double containing easting or longitude
 * \param y Pointer to a double containing northing or latitude
 * \param z Pointer to a double containing height, or NULL
 *
 * \return Return value from PROJ proj_trans() function
 **/

int GPJ_transform(const struct pj_info *info_in,
                  const struct pj_info *info_out,
                  const struct pj_info *info_trans, int dir,
		  double *x, double *y, double *z)
{
    int ok = 0;

#ifdef HAVE_PROJ_H
    /* PROJ 5+ variant */
    int in_is_ll, out_is_ll, in_deg2rad, out_rad2deg;
    PJ_COORD c;

    if (info_in->pj == NULL)
	G_fatal_error(_("No input projection"));

    if (info_trans->pj == NULL)
	G_fatal_error(_("No transformation object"));

    in_deg2rad = out_rad2deg = 1;
    if (dir == PJ_FWD) {
	/* info_in -> info_out */
	METERS_in = info_in->meters;
	in_is_ll = !strncmp(info_in->proj, "ll", 2);
#if PROJ_VERSION_MAJOR >= 6
	/* PROJ 6+: conversion to radians is not always needed:
	 * if proj_angular_input(info_trans->pj, dir) == 1
	 * -> convert from degrees to radians */
	if (in_is_ll && proj_angular_input(info_trans->pj, dir) == 0) {
	    in_deg2rad = 0;
	}
#endif
	if (info_out->pj) {
	    METERS_out = info_out->meters;
	    out_is_ll = !strncmp(info_out->proj, "ll", 2);
#if PROJ_VERSION_MAJOR >= 6
	    /* PROJ 6+: conversion to radians is not always needed:
	     * if proj_angular_input(info_trans->pj, dir) == 1
	     * -> convert from degrees to radians */
	    if (out_is_ll && proj_angular_output(info_trans->pj, dir) == 0) {
		out_rad2deg = 0;
	    }
#endif
	}
	else {
	    METERS_out = 1.0;
	    out_is_ll = 1;
	}
    }
    else {
	/* info_out -> info_in */
	METERS_out = info_in->meters;
	out_is_ll = !strncmp(info_in->proj, "ll", 2);
#if PROJ_VERSION_MAJOR >= 6
	/* PROJ 6+: conversion to radians is not always needed:
	 * if proj_angular_input(info_trans->pj, dir) == 1
	 * -> convert from degrees to radians */
	if (out_is_ll && proj_angular_output(info_trans->pj, dir) == 0) {
	    out_rad2deg = 0;
	}
#endif
	if (info_out->pj) {
	    METERS_in = info_out->meters;
	    in_is_ll = !strncmp(info_out->proj, "ll", 2);
#if PROJ_VERSION_MAJOR >= 6
	    /* PROJ 6+: conversion to radians is not always needed:
	     * if proj_angular_input(info_trans->pj, dir) == 1
	     * -> convert from degrees to radians */
	    if (in_is_ll && proj_angular_input(info_trans->pj, dir) == 0) {
		in_deg2rad = 0;
	    }
#endif
	}
	else {
	    METERS_in = 1.0;
	    in_is_ll = 1;
	}
    }

    /* prepare */
    if (in_is_ll) {
	if (in_deg2rad) {
	    /* convert degrees to radians */
	    c.lpzt.lam = (*x) / RAD_TO_DEG;
	    c.lpzt.phi = (*y) / RAD_TO_DEG;
	}
	else {
	    c.lpzt.lam = (*x);
	    c.lpzt.phi = (*y);
	}
	c.lpzt.z = 0;
	if (z)
	    c.lpzt.z = *z;
	c.lpzt.t = 0;
    }
    else {
	/* convert to meters */
	c.xyzt.x = *x * METERS_in;
	c.xyzt.y = *y * METERS_in;
	c.xyzt.z = 0;
	if (z)
	    c.xyzt.z = *z;
	c.xyzt.t = 0;
    }

    G_debug(1, "c.xyzt.x: %g", c.xyzt.x);
    G_debug(1, "c.xyzt.y: %g", c.xyzt.y);
    G_debug(1, "c.xyzt.z: %g", c.xyzt.z);

    /* transform */
    c = proj_trans(info_trans->pj, dir, c);
    ok = proj_errno(info_trans->pj);

    if (ok < 0) {
	G_warning(_("proj_trans() failed: %s"), proj_errno_string(ok));
	return ok;
    }

    /* output */
    if (out_is_ll) {
	/* convert to degrees */
	if (out_rad2deg) {
	    /* convert radians to degrees */
	    *x = c.lp.lam * RAD_TO_DEG;
	    *y = c.lp.phi * RAD_TO_DEG;
	}
	else {
	    *x = c.lp.lam;
	    *y = c.lp.phi;
	}
	if (z)
	    *z = c.lpzt.z;
    }
    else {
	/* convert to map units */
	*x = c.xyzt.x / METERS_out;
	*y = c.xyzt.y / METERS_out;
	if (z)
	    *z = c.xyzt.z;
    }
#else
    /* PROJ 4 variant */
    double u, v;
    double h = 0.0;
    const struct pj_info *p_in, *p_out;

    if (info_out == NULL)
	G_fatal_error(_("No output projection"));

    if (dir == PJ_FWD) {
	p_in = info_in;
	p_out = info_out;
    }
    else {
	p_in = info_out;
	p_out = info_in;
    }

    METERS_in = p_in->meters;
    METERS_out = p_out->meters;

    if (z)
	h = *z;

    if (strncmp(p_in->proj, "ll", 2) == 0) {
	u = (*x) / RAD_TO_DEG;
	v = (*y) / RAD_TO_DEG;
    }
    else {
	u = *x * METERS_in;
	v = *y * METERS_in;
    }

    ok = pj_transform(p_in->pj, p_out->pj, 1, 0, &u, &v, &h);

    if (ok < 0) {
	G_warning(_("pj_transform() failed: %s"), pj_strerrno(ok));
	return ok;
    }

    if (strncmp(p_out->proj, "ll", 2) == 0) {
	*x = u * RAD_TO_DEG;
	*y = v * RAD_TO_DEG;
    }
    else {
	*x = u / METERS_out;
	*y = v / METERS_out;
    }
    if (z)
	*z = h;
#endif

    return ok;
}

/**
 * \brief Re-project an array of points between two co-ordinate systems
 *        using a transformation object prepared with GPJ_prepare_pj()
 *
 * This function takes pointers to three pj_info structures as arguments,
 * and projects an array of pointd between the input and output
 * co-ordinate system. The pj_info structure info_trans must have been
 * initialized with GPJ_init_transform().
 * The direction determines if a point is projected from input CRS to
 * output CRS (PJ_FWD) or from output CRS to input CRS (PJ_INV).
 * The easting, northing, and height of the point are contained in the
 * pointers passed to the function; these will be overwritten by the
 * coordinates of the transformed point.
 *
 * \param info_in pointer to pj_info struct for input co-ordinate system
 * \param info_out pointer to pj_info struct for output co-ordinate system
 * \param info_trans pointer to pj_info struct for a transformation object (PROJ 5+)
 * \param dir direction of the transformation (PJ_FWD or PJ_INV)
 * \param x pointer to an array of type double containing easting or longitude
 * \param y pointer to an array of type double containing northing or latitude
 * \param z pointer to an array of type double containing height, or NULL
 * \param n number of points in the arrays to be transformed
 *
 * \return Return value from PROJ proj_trans() function
 **/

int GPJ_transform_array(const struct pj_info *info_in,
                        const struct pj_info *info_out,
                        const struct pj_info *info_trans, int dir,
		        double *x, double *y, double *z, int n)
{
    int ok;
    int i;
    int has_z = 1;

#ifdef HAVE_PROJ_H
    /* PROJ 5+ variant */
    int in_is_ll, out_is_ll, in_deg2rad, out_rad2deg;
    PJ_COORD c;

    if (info_trans->pj == NULL)
	G_fatal_error(_("No transformation object"));

    in_deg2rad = out_rad2deg = 1;
    if (dir == PJ_FWD) {
	/* info_in -> info_out */
	METERS_in = info_in->meters;
	in_is_ll = !strncmp(info_in->proj, "ll", 2);
#if PROJ_VERSION_MAJOR >= 6
	/* PROJ 6+: conversion to radians is not always needed:
	 * if proj_angular_input(info_trans->pj, dir) == 1
	 * -> convert from degrees to radians */
	if (in_is_ll && proj_angular_input(info_trans->pj, dir) == 0) {
	    in_deg2rad = 0;
	}
#endif
	if (info_out->pj) {
	    METERS_out = info_out->meters;
	    out_is_ll = !strncmp(info_out->proj, "ll", 2);
#if PROJ_VERSION_MAJOR >= 6
	    /* PROJ 6+: conversion to radians is not always needed:
	     * if proj_angular_input(info_trans->pj, dir) == 1
	     * -> convert from degrees to radians */
	    if (out_is_ll && proj_angular_output(info_trans->pj, dir) == 0) {
		out_rad2deg = 0;
	    }
#endif
	}
	else {
	    METERS_out = 1.0;
	    out_is_ll = 1;
	}
    }
    else {
	/* info_out -> info_in */
	METERS_out = info_in->meters;
	out_is_ll = !strncmp(info_in->proj, "ll", 2);
#if PROJ_VERSION_MAJOR >= 6
	/* PROJ 6+: conversion to radians is not always needed:
	 * if proj_angular_input(info_trans->pj, dir) == 1
	 * -> convert from degrees to radians */
	if (out_is_ll && proj_angular_output(info_trans->pj, dir) == 0) {
	    out_rad2deg = 0;
	}
#endif
	if (info_out->pj) {
	    METERS_in = info_out->meters;
	    in_is_ll = !strncmp(info_out->proj, "ll", 2);
#if PROJ_VERSION_MAJOR >= 6
	    /* PROJ 6+: conversion to degrees is not always needed:
	     * if proj_angular_output(info_trans->pj, dir) == 1
	     * -> convert from degrees to radians */
	    if (in_is_ll && proj_angular_input(info_trans->pj, dir) == 0) {
		in_deg2rad = 0;
	    }
#endif
	}
	else {
	    METERS_in = 1.0;
	    in_is_ll = 1;
	}
    }

    if (z == NULL) {
	z = G_malloc(sizeof(double) * n);
	/* they say memset is only guaranteed for chars ;-( */
	for (i = 0; i < n; i++)
	    z[i] = 0.0;
	has_z = 0;
    }
    ok = 0;
    if (in_is_ll) {
	c.lpzt.t = 0;
	if (out_is_ll) {
	    /* what is more costly ?
	     * calling proj_trans for each point
	     * or having three loops over all points ?
	     * proj_trans_array() itself calls proj_trans() in a loop
	     * -> one loop over all points is better than
	     *    three loops over all points
	     */
	    for (i = 0; i < n; i++) {
		if (in_deg2rad) {
		    /* convert degrees to radians */
		    c.lpzt.lam = (*x) / RAD_TO_DEG;
		    c.lpzt.phi = (*y) / RAD_TO_DEG;
		}
		else {
		    c.lpzt.lam = (*x);
		    c.lpzt.phi = (*y);
		}
		c.lpzt.z = z[i];
		c = proj_trans(info_trans->pj, dir, c);
		if ((ok = proj_errno(info_trans->pj)) < 0)
		    break;
		if (out_rad2deg) {
		    /* convert radians to degrees */
		    x[i] = c.lp.lam * RAD_TO_DEG;
		    y[i] = c.lp.phi * RAD_TO_DEG;
		}
		else {
		    x[i] = c.lp.lam;
		    y[i] = c.lp.phi;
		}
	    }
	}
	else {
	    for (i = 0; i < n; i++) {
		if (in_deg2rad) {
		    /* convert degrees to radians */
		    c.lpzt.lam = (*x) / RAD_TO_DEG;
		    c.lpzt.phi = (*y) / RAD_TO_DEG;
		}
		else {
		    c.lpzt.lam = (*x);
		    c.lpzt.phi = (*y);
		}
		c.lpzt.z = z[i];
		c = proj_trans(info_trans->pj, dir, c);
		if ((ok = proj_errno(info_trans->pj)) < 0)
		    break;

		/* convert to map units */
		x[i] = c.xy.x / METERS_out;
		y[i] = c.xy.y / METERS_out;
	    }
	}
    }
    else {
	c.xyzt.t = 0;
	if (out_is_ll) {
	    for (i = 0; i < n; i++) {
		/* convert to meters */
		c.xyzt.x = x[i] * METERS_in;
		c.xyzt.y = y[i] * METERS_in;
		c.xyzt.z = z[i];
		c = proj_trans(info_trans->pj, dir, c);
		if ((ok = proj_errno(info_trans->pj)) < 0)
		    break;
		if (out_rad2deg) {
		    /* convert radians to degrees */
		    x[i] = c.lp.lam * RAD_TO_DEG;
		    y[i] = c.lp.phi * RAD_TO_DEG;
		}
		else {
		    x[i] = c.lp.lam;
		    y[i] = c.lp.phi;
		}
	    }
	}
	else {
	    for (i = 0; i < n; i++) {
		/* convert to meters */
		c.xyzt.x = x[i] * METERS_in;
		c.xyzt.y = y[i] * METERS_in;
		c.xyzt.z = z[i];
		c = proj_trans(info_trans->pj, dir, c);
		if ((ok = proj_errno(info_trans->pj)) < 0)
		    break;
		/* convert to map units */
		x[i] = c.xy.x / METERS_out;
		y[i] = c.xy.y / METERS_out;
	    }
	}
    }
    if (!has_z)
	G_free(z);

    if (ok < 0) {
	G_warning(_("proj_trans() failed: %s"), proj_errno_string(ok));
    }
#else
    /* PROJ 4 variant */
    const struct pj_info *p_in, *p_out;

    if (dir == PJ_FWD) {
	p_in = info_in;
	p_out = info_out;
    }
    else {
	p_in = info_out;
	p_out = info_in;
    }

    METERS_in = p_in->meters;
    METERS_out = p_out->meters;

    if (z == NULL) {
	z = G_malloc(sizeof(double) * n);
	/* they say memset is only guaranteed for chars ;-( */
	for (i = 0; i < n; ++i)
	    z[i] = 0.0;
	has_z = 0;
    }
    if (strncmp(p_in->proj, "ll", 2) == 0) {
	if (strncmp(p_out->proj, "ll", 2) == 0) {
	    DIVIDE_LOOP(x, y, n, RAD_TO_DEG);
	    ok = pj_transform(info_in->pj, info_out->pj, n, 1, x, y, z);
	    MULTIPLY_LOOP(x, y, n, RAD_TO_DEG);
	}
	else {
	    DIVIDE_LOOP(x, y, n, RAD_TO_DEG);
	    ok = pj_transform(info_in->pj, info_out->pj, n, 1, x, y, z);
	    DIVIDE_LOOP(x, y, n, METERS_out);
	}
    }
    else {
	if (strncmp(p_out->proj, "ll", 2) == 0) {
	    MULTIPLY_LOOP(x, y, n, METERS_in);
	    ok = pj_transform(info_in->pj, info_out->pj, n, 1, x, y, z);
	    MULTIPLY_LOOP(x, y, n, RAD_TO_DEG);
	}
	else {
	    MULTIPLY_LOOP(x, y, n, METERS_in);
	    ok = pj_transform(info_in->pj, info_out->pj, n, 1, x, y, z);
	    DIVIDE_LOOP(x, y, n, METERS_out);
	}
    }
    if (!has_z)
	G_free(z);

    if (ok < 0)
	G_warning(_("pj_transform() failed: %s"), pj_strerrno(ok));
#endif

    return ok;
}

/*
 * old API, to be deleted
 */

/**
 * \brief Re-project a point between two co-ordinate systems
 *
 * This function takes pointers to two pj_info structures as arguments,
 * and projects a point between the co-ordinate systems represented by them.
 * The easting and northing of the point are contained in two pointers passed
 * to the function; these will be overwritten by the co-ordinates of the
 * re-projected point.
 *
 * \param x Pointer to a double containing easting or longitude
 * \param y Pointer to a double containing northing or latitude
 * \param info_in pointer to pj_info struct for input co-ordinate system
 * \param info_out pointer to pj_info struct for output co-ordinate system
 *
 * \return Return value from PROJ proj_trans() function
 **/

int pj_do_proj(double *x, double *y,
	       const struct pj_info *info_in, const struct pj_info *info_out)
{
    int ok;
#ifdef HAVE_PROJ_H
    struct pj_info info_trans;
    PJ_COORD c;

    if (GPJ_init_transform(info_in, info_out, &info_trans) < 0) {
	return -1;
    }

    METERS_in = info_in->meters;
    METERS_out = info_out->meters;

    if (strncmp(info_in->proj, "ll", 2) == 0) {
	/* convert to radians */
	c.lpzt.lam = (*x) / RAD_TO_DEG;
	c.lpzt.phi = (*y) / RAD_TO_DEG;
	c.lpzt.z = 0;
	c.lpzt.t = 0;
	c = proj_trans(info_trans.pj, PJ_FWD, c);
	ok = proj_errno(info_trans.pj);

	if (strncmp(info_out->proj, "ll", 2) == 0) {
	    /* convert to degrees */
	    *x = c.lp.lam * RAD_TO_DEG;
	    *y = c.lp.phi * RAD_TO_DEG;
	}
	else {
	    /* convert to map units */
	    *x = c.xy.x / METERS_out;
	    *y = c.xy.y / METERS_out;
	}
    }
    else {
	/* convert to meters */
	c.xyzt.x = *x * METERS_in;
	c.xyzt.y = *y * METERS_in;
	c.xyzt.z = 0;
	c.xyzt.t = 0;
	c = proj_trans(info_trans.pj, PJ_FWD, c);
	ok = proj_errno(info_trans.pj);

	if (strncmp(info_out->proj, "ll", 2) == 0) {
	    /* convert to degrees */
	    *x = c.lp.lam * RAD_TO_DEG;
	    *y = c.lp.phi * RAD_TO_DEG;
	}
	else {
	    /* convert to map units */
	    *x = c.xy.x / METERS_out;
	    *y = c.xy.y / METERS_out;
	}
    }
    proj_destroy(info_trans.pj);

    if (ok < 0) {
	G_warning(_("proj_trans() failed: %d"), ok);
    }
#else
    double u, v;
    double h = 0.0;

    METERS_in = info_in->meters;
    METERS_out = info_out->meters;

    if (strncmp(info_in->proj, "ll", 2) == 0) {
	if (strncmp(info_out->proj, "ll", 2) == 0) {
	    u = (*x) / RAD_TO_DEG;
	    v = (*y) / RAD_TO_DEG;
	    ok = pj_transform(info_in->pj, info_out->pj, 1, 0, &u, &v, &h);
	    *x = u * RAD_TO_DEG;
	    *y = v * RAD_TO_DEG;
	}
	else {
	    u = (*x) / RAD_TO_DEG;
	    v = (*y) / RAD_TO_DEG;
	    ok = pj_transform(info_in->pj, info_out->pj, 1, 0, &u, &v, &h);
	    *x = u / METERS_out;
	    *y = v / METERS_out;
	}
    }
    else {
	if (strncmp(info_out->proj, "ll", 2) == 0) {
	    u = *x * METERS_in;
	    v = *y * METERS_in;
	    ok = pj_transform(info_in->pj, info_out->pj, 1, 0, &u, &v, &h);
	    *x = u * RAD_TO_DEG;
	    *y = v * RAD_TO_DEG;
	}
	else {
	    u = *x * METERS_in;
	    v = *y * METERS_in;
	    ok = pj_transform(info_in->pj, info_out->pj, 1, 0, &u, &v, &h);
	    *x = u / METERS_out;
	    *y = v / METERS_out;
	}
    }
    if (ok < 0) {
	G_warning(_("pj_transform() failed: %s"), pj_strerrno(ok));
    }
#endif
    return ok;
}

/**
 * \brief Re-project an array of points between two co-ordinate systems with
 *        optional ellipsoidal height conversion
 *
 * This function takes pointers to two pj_info structures as arguments,
 * and projects an array of points between the co-ordinate systems
 * represented by them. Pointers to the three arrays of easting, northing,
 * and ellipsoidal height of the point (this one may be NULL) are passed
 * to the function; these will be overwritten by the co-ordinates of the
 * re-projected points.
 *
 * \param count Number of points in the arrays to be transformed
 * \param x Pointer to an array of type double containing easting or longitude
 * \param y Pointer to an array of type double containing northing or latitude
 * \param h Pointer to an array of type double containing ellipsoidal height.
 *          May be null in which case a two-dimensional re-projection will be
 *          done
 * \param info_in pointer to pj_info struct for input co-ordinate system
 * \param info_out pointer to pj_info struct for output co-ordinate system
 *
 * \return Return value from PROJ proj_trans() function
 **/

int pj_do_transform(int count, double *x, double *y, double *h,
		    const struct pj_info *info_in, const struct pj_info *info_out)
{
    int ok;
    int i;
    int has_h = 1;
#ifdef HAVE_PROJ_H
    struct pj_info info_trans;
    PJ_COORD c;

    if (GPJ_init_transform(info_in, info_out, &info_trans) < 0) {
	return -1;
    }

    METERS_in = info_in->meters;
    METERS_out = info_out->meters;

    if (h == NULL) {
	h = G_malloc(sizeof *h * count);
	/* they say memset is only guaranteed for chars ;-( */
	for (i = 0; i < count; ++i)
	    h[i] = 0.0;
	has_h = 0;
    }
    ok = 0;
    if (strncmp(info_in->proj, "ll", 2) == 0) {
	c.lpzt.t = 0;
	if (strncmp(info_out->proj, "ll", 2) == 0) {
	    for (i = 0; i < count; i++) {
		/* convert to radians */
		c.lpzt.lam = x[i] / RAD_TO_DEG;
		c.lpzt.phi = y[i] / RAD_TO_DEG;
		c.lpzt.z = h[i];
		c = proj_trans(info_trans.pj, PJ_FWD, c);
		if ((ok = proj_errno(info_trans.pj)) < 0)
		    break;
		/* convert to degrees */
		x[i] = c.lp.lam * RAD_TO_DEG;
		y[i] = c.lp.phi * RAD_TO_DEG;
	    }
	}
	else {
	    for (i = 0; i < count; i++) {
		/* convert to radians */
		c.lpzt.lam = x[i] / RAD_TO_DEG;
		c.lpzt.phi = y[i] / RAD_TO_DEG;
		c.lpzt.z = h[i];
		c = proj_trans(info_trans.pj, PJ_FWD, c);
		if ((ok = proj_errno(info_trans.pj)) < 0)
		    break;
		/* convert to map units */
		x[i] = c.xy.x / METERS_out;
		y[i] = c.xy.y / METERS_out;
	    }
	}
    }
    else {
	c.xyzt.t = 0;
	if (strncmp(info_out->proj, "ll", 2) == 0) {
	    for (i = 0; i < count; i++) {
		/* convert to meters */
		c.xyzt.x = x[i] * METERS_in;
		c.xyzt.y = y[i] * METERS_in;
		c.xyzt.z = h[i];
		c = proj_trans(info_trans.pj, PJ_FWD, c);
		if ((ok = proj_errno(info_trans.pj)) < 0)
		    break;
		/* convert to degrees */
		x[i] = c.lp.lam * RAD_TO_DEG;
		y[i] = c.lp.phi * RAD_TO_DEG;
	    }
	}
	else {
	    for (i = 0; i < count; i++) {
		/* convert to meters */
		c.xyzt.x = x[i] * METERS_in;
		c.xyzt.y = y[i] * METERS_in;
		c.xyzt.z = h[i];
		c = proj_trans(info_trans.pj, PJ_FWD, c);
		if ((ok = proj_errno(info_trans.pj)) < 0)
		    break;
		/* convert to map units */
		x[i] = c.xy.x / METERS_out;
		y[i] = c.xy.y / METERS_out;
	    }
	}
    }
    if (!has_h)
	G_free(h);
    proj_destroy(info_trans.pj);

    if (ok < 0) {
	G_warning(_("proj_trans() failed: %d"), ok);
    }
#else
    METERS_in = info_in->meters;
    METERS_out = info_out->meters;

    if (h == NULL) {
	h = G_malloc(sizeof *h * count);
	/* they say memset is only guaranteed for chars ;-( */
	for (i = 0; i < count; ++i)
	    h[i] = 0.0;
	has_h = 0;
    }
    if (strncmp(info_in->proj, "ll", 2) == 0) {
	if (strncmp(info_out->proj, "ll", 2) == 0) {
	    DIVIDE_LOOP(x, y, count, RAD_TO_DEG);
	    ok = pj_transform(info_in->pj, info_out->pj, count, 1, x, y, h);
	    MULTIPLY_LOOP(x, y, count, RAD_TO_DEG);
	}
	else {
	    DIVIDE_LOOP(x, y, count, RAD_TO_DEG);
	    ok = pj_transform(info_in->pj, info_out->pj, count, 1, x, y, h);
	    DIVIDE_LOOP(x, y, count, METERS_out);
	}
    }
    else {
	if (strncmp(info_out->proj, "ll", 2) == 0) {
	    MULTIPLY_LOOP(x, y, count, METERS_in);
	    ok = pj_transform(info_in->pj, info_out->pj, count, 1, x, y, h);
	    MULTIPLY_LOOP(x, y, count, RAD_TO_DEG);
	}
	else {
	    MULTIPLY_LOOP(x, y, count, METERS_in);
	    ok = pj_transform(info_in->pj, info_out->pj, count, 1, x, y, h);
	    DIVIDE_LOOP(x, y, count, METERS_out);
	}
    }
    if (!has_h)
	G_free(h);

    if (ok < 0) {
	G_warning(_("pj_transform() failed: %s"), pj_strerrno(ok));
    }
#endif
    return ok;
}
