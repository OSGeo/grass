
/**
   \file get_proj.c

   \brief GProj library - Functions for re-projecting point data

   \author Original Author unknown, probably Soil Conservation Service,
   Eric Miller, Paul Kelly, Markus Metz

   (C) 2003-2008, 2018 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.
**/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

/* Finder function for datum transformation grids */
#define FINDERFUNC set_proj_share
#define PERMANENT "PERMANENT"
#define MAX_PARGS 100

static void alloc_options(char *);

static char *opt_in[MAX_PARGS];
static int nopt;

/* TODO: rename pj_ to GPJ_ to avoid symbol clash with PROJ lib */

/**
 * \brief Create a pj_info struct Co-ordinate System definition from a set of
 *        PROJ_INFO / PROJ_UNITS-style key-value pairs
 *
 * This function takes a GRASS-style co-ordinate system definition as stored
 * in the PROJ_INFO and PROJ_UNITS files and processes it to create a pj_info
 * representation for use in re-projecting with pj_do_proj(). In addition to
 * the parameters passed to it it may also make reference to the system
 * ellipse.table and datum.table files if necessary.
 *
 * \param info Pointer to a pj_info struct (which must already exist) into
 *        which the co-ordinate system definition will be placed
 * \param in_proj_keys PROJ_INFO-style key-value pairs
 * \param in_units_keys PROJ_UNITS-style key-value pairs
 *
 * \return -1 on error (unable to initialise PROJ.4)
 *          2 if "default" 3-parameter datum shift values from datum.table
 *            were used
 *          3 if an unrecognised datum name was passed on to PROJ.4 (and
 *            initialization was successful)
 *          1 otherwise
 **/

int pj_get_kv(struct pj_info *info, const struct Key_Value *in_proj_keys,
	      const struct Key_Value *in_units_keys)
{
    const char *str;
    int i;
    double a, es, rf;
    int returnval = 1;
    char buffa[300], factbuff[50];
    int deflen;
    char proj_in[250], *datum, *params;
#ifdef HAVE_PROJ_H
    PJ *pj;
    PJ_CONTEXT *pjc;
#else
    projPJ *pj;
#endif

    proj_in[0] = '\0';
    info->zone = 0;
    info->meters = 1.0;
    info->proj[0] = '\0';
    info->def = NULL;
    info->pj = NULL;
    info->srid = NULL;
    info->wkt = NULL;

    str = G_find_key_value("meters", in_units_keys);
    if (str != NULL) {
	strcpy(factbuff, str);
	if (strlen(factbuff) > 0)
	    sscanf(factbuff, "%lf", &(info->meters));
    }
    str = G_find_key_value("name", in_proj_keys);
    if (str != NULL) {
	sprintf(proj_in, "%s", str);
    }
    str = G_find_key_value("proj", in_proj_keys);
    if (str != NULL) {
	sprintf(info->proj, "%s", str);
    }
    if (strlen(info->proj) <= 0)
	sprintf(info->proj, "ll");
    str = G_find_key_value("init", in_proj_keys);
    if (str != NULL) {
	info->srid = G_store(str);
    }

    nopt = 0;
    for (i = 0; i < in_proj_keys->nitems; i++) {
	/* the name parameter is just for grasses use */
	if (strcmp(in_proj_keys->key[i], "name") == 0) {
	    continue;

	    /* init is here ignored */
	}
	else if (strcmp(in_proj_keys->key[i], "init") == 0) {
	    continue;

	    /* zone handled separately at end of loop */
	}
	else if (strcmp(in_proj_keys->key[i], "zone") == 0) {
	    continue;

	    /* Datum and ellipsoid-related parameters will be handled
	     * separately after end of this loop PK */

	}
	else if (strcmp(in_proj_keys->key[i], "datum") == 0
		 || strcmp(in_proj_keys->key[i], "dx") == 0
		 || strcmp(in_proj_keys->key[i], "dy") == 0
		 || strcmp(in_proj_keys->key[i], "dz") == 0
		 || strcmp(in_proj_keys->key[i], "datumparams") == 0
		 || strcmp(in_proj_keys->key[i], "nadgrids") == 0
		 || strcmp(in_proj_keys->key[i], "towgs84") == 0
		 || strcmp(in_proj_keys->key[i], "ellps") == 0
		 || strcmp(in_proj_keys->key[i], "a") == 0
		 || strcmp(in_proj_keys->key[i], "b") == 0
		 || strcmp(in_proj_keys->key[i], "es") == 0
		 || strcmp(in_proj_keys->key[i], "f") == 0
		 || strcmp(in_proj_keys->key[i], "rf") == 0) {
	    continue;

	    /* PROJ.4 uses longlat instead of ll as 'projection name' */

	}
	else if (strcmp(in_proj_keys->key[i], "proj") == 0) {
	    if (strcmp(in_proj_keys->value[i], "ll") == 0)
		sprintf(buffa, "proj=longlat");
	    else
		sprintf(buffa, "proj=%s", in_proj_keys->value[i]);

	    /* 'One-sided' PROJ.4 flags will have the value in
	     * the key-value pair set to 'defined' and only the
	     * key needs to be passed on. */
	}
	else if (strcmp(in_proj_keys->value[i], "defined") == 0)
	    sprintf(buffa, "%s", in_proj_keys->key[i]);

	else
	    sprintf(buffa, "%s=%s",
		    in_proj_keys->key[i], in_proj_keys->value[i]);

	alloc_options(buffa);
    }

    str = G_find_key_value("zone", in_proj_keys);
    if (str != NULL) {
	if (sscanf(str, "%d", &(info->zone)) != 1) {
	    G_fatal_error(_("Invalid zone %s specified"), str);
	}
	if (info->zone < 0) {

	    /* if zone is negative, write abs(zone) and define south */
	    info->zone = -info->zone;

	    if (G_find_key_value("south", in_proj_keys) == NULL) {
		sprintf(buffa, "south");
		alloc_options(buffa);
	    }
	}
	sprintf(buffa, "zone=%d", info->zone);
	alloc_options(buffa);
    }

    if ((GPJ__get_ellipsoid_params(in_proj_keys, &a, &es, &rf) == 0)
	&& (str = G_find_key_value("ellps", in_proj_keys)) != NULL) {
	/* Default values were returned but an ellipsoid name not recognised
	 * by GRASS is present---perhaps it will be recognised by
	 * PROJ.4 even though it wasn't by GRASS */
	sprintf(buffa, "ellps=%s", str);
	alloc_options(buffa);
    }
    else {
	sprintf(buffa, "a=%.16g", a);
	alloc_options(buffa);
	/* Cannot use es directly because the OSRImportFromProj4()
	 * function in OGR only accepts b or rf as the 2nd parameter */
	if (es == 0)
	    sprintf(buffa, "b=%.16g", a);
	else
	    sprintf(buffa, "rf=%.16g", rf);
	alloc_options(buffa);

    }
    /* Workaround to stop PROJ reading values from defaults file when
     * rf (and sometimes ellps) is not specified */
    if (G_find_key_value("no_defs", in_proj_keys) == NULL) {
	sprintf(buffa, "no_defs");
	alloc_options(buffa);
    }

    /* If datum parameters are present in the PROJ_INFO keys, pass them on */
    if (GPJ__get_datum_params(in_proj_keys, &datum, &params) == 2) {
	sprintf(buffa, "%s", params);
	alloc_options(buffa);
	G_free(params);

	/* else if a datum name is present take it and look up the parameters
	 * from the datum.table file */
    }
    else if (datum != NULL) {

	if (GPJ_get_default_datum_params_by_name(datum, &params) > 0) {
	    sprintf(buffa, "%s", params);
	    alloc_options(buffa);
	    returnval = 2;
	    G_free(params);

	    /* else just pass the datum name on and hope it is recognised by
	     * PROJ.4 even though it isn't recognised by GRASS */
	}
	else {
	    sprintf(buffa, "datum=%s", datum);
	    alloc_options(buffa);
	    returnval = 3;
	}
	/* else there'll be no datum transformation taking place here... */
    }
    else {
	returnval = 4;
    }
    G_free(datum);

#ifdef HAVE_PROJ_H
#if PROJ_VERSION_MAJOR >= 6
    /* without type=crs, PROJ6 does not recognize what this is,
     * a crs or some kind of coordinate operation, falling through to
     * PJ_TYPE_OTHER_COORDINATE_OPERATION */
    alloc_options("type=crs");
#endif
    pjc = proj_context_create();
    if (!(pj = proj_create_argv(pjc, nopt, opt_in))) {
#else
    /* Set finder function for locating datum conversion tables PK */
    pj_set_finder(FINDERFUNC);

    if (!(pj = pj_init(nopt, opt_in))) {
#endif
	strcpy(buffa,
	       _("Unable to initialise PROJ with the following parameter list:"));
	for (i = 0; i < nopt; i++) {
	    char err[50];

	    sprintf(err, " +%s", opt_in[i]);
	    strcat(buffa, err);
	}
	G_warning("%s", buffa);
#ifndef HAVE_PROJ_H
	G_warning(_("The PROJ error message: %s"), pj_strerrno(pj_errno));
#endif
	return -1;
    }

#ifdef HAVE_PROJ_H
    int perr = proj_errno(pj);

    if (perr)
	G_fatal_error("PROJ 5 error %d", perr);
#endif

    info->pj = pj;

    deflen = 0;
    for (i = 0; i < nopt; i++)
	deflen += strlen(opt_in[i]) + 2;

    info->def = G_malloc(deflen + 1);

    sprintf(buffa,  "+%s ", opt_in[0]);
    strcpy(info->def, buffa);
    G_free(opt_in[0]);

    for (i = 1; i < nopt; i++) {
	sprintf(buffa,  "+%s ", opt_in[i]);
	strcat(info->def, buffa);
	G_free(opt_in[i]);
    }

    return returnval;
}

static void alloc_options(char *buffa)
{
    int nsize;

    nsize = strlen(buffa);
    opt_in[nopt++] = (char *)G_malloc(nsize + 1);
    sprintf(opt_in[nopt - 1], "%s", buffa);
    return;
}

/**
 * \brief Create a pj_info struct Co-ordinate System definition from a
 *        string with a sequence of key=value pairs
 *
 * This function takes a GRASS- or PROJ style co-ordinate system definition
 * and processes it to create a pj_info representation for use in
 * re-projecting with pj_do_proj(). In addition to the parameters passed
 * to it it may also make reference to the system ellipse.table and
 * datum.table files if necessary.
 *
 * \param info Pointer to a pj_info struct (which must already exist) into
 *        which the co-ordinate system definition will be placed
 * \param str input string with projection definition
 * \param in_units_keys PROJ_UNITS-style key-value pairs
 *
 * \return -1 on error (unable to initialise PROJ.4)
 *          1 on success
 **/

int pj_get_string(struct pj_info *info, char *str)
{
    char *s;
    int i, nsize;
    char zonebuff[50], buffa[300];
    int deflen;
#ifdef HAVE_PROJ_H
    PJ *pj;
    PJ_CONTEXT *pjc;
#else
    projPJ *pj;
#endif

    info->zone = 0;
    info->proj[0] = '\0';
    info->meters = 1.0;
    info->def = NULL;
    info->srid = NULL;
    info->pj = NULL;

    nopt = 0;

    if ((str == NULL) || (str[0] == '\0')) {
	/* Null Pointer or empty string is supplied for parameters,
	 * implying latlong projection; just need to set proj
	 * parameter and call pj_init PK */
	sprintf(info->proj, "ll");
	sprintf(buffa, "proj=latlong ellps=WGS84");
	alloc_options(buffa);
    }
    else {
	/* Parameters have been provided; parse through them but don't
	 * bother with most of the checks in pj_get_kv; assume the
	 * programmer knows what he / she is doing when using this
	 * function rather than reading a PROJ_INFO file       PK */
	s = str;
	while (s = strtok(s, " \t\n"), s) {
	    if (strncmp(s, "+unfact=", 8) == 0) {
		s = s + 8;
		info->meters = atof(s);
	    }
	    else {
		if (strncmp(s, "+", 1) == 0)
		    ++s;
		if (nsize = strlen(s), nsize) {
		    if (nopt >= MAX_PARGS) {
			fprintf(stderr, "nopt = %d, s=%s\n", nopt, str);
			G_fatal_error(_("Option input overflowed option table"));
		    }

		    if (strncmp("zone=", s, 5) == 0) {
			sprintf(zonebuff, "%s", s + 5);
			sscanf(zonebuff, "%d", &(info->zone));
		    }

		    if (strncmp(s, "init=", 5) == 0) {
			info->srid = G_store(s + 6);
		    }

		    if (strncmp("proj=", s, 5) == 0) {
			sprintf(info->proj, "%s", s + 5);
			if (strcmp(info->proj, "ll") == 0)
			    sprintf(buffa, "proj=latlong");
			else
			    sprintf(buffa, "%s", s);
		    }
		    else {
			sprintf(buffa, "%s", s);
		    }
		    alloc_options(buffa);
		}
	    }
	    s = 0;
	}
    }

#ifdef HAVE_PROJ_H
#if PROJ_VERSION_MAJOR >= 6
    /* without type=crs, PROJ6 does not recognize what this is,
     * a crs or some kind of coordinate operation, falling through to
     * PJ_TYPE_OTHER_COORDINATE_OPERATION */
    alloc_options("type=crs");
#endif
    pjc = proj_context_create();
    if (!(pj = proj_create_argv(pjc, nopt, opt_in))) {
	G_warning(_("Unable to initialize pj cause: %s"),
	          proj_errno_string(proj_context_errno(pjc)));
	return -1;
    }
#else
    /* Set finder function for locating datum conversion tables PK */
    pj_set_finder(FINDERFUNC);

    if (!(pj = pj_init(nopt, opt_in))) {
	G_warning(_("Unable to initialize pj cause: %s"),
		  pj_strerrno(pj_errno));
	return -1;
    }
#endif
    info->pj = pj;

    deflen = 0;
    for (i = 0; i < nopt; i++)
	deflen += strlen(opt_in[i]) + 2;

    info->def = G_malloc(deflen + 1);

    sprintf(buffa,  "+%s ", opt_in[0]);
    strcpy(info->def, buffa);
    G_free(opt_in[0]);

    for (i = 1; i < nopt; i++) {
	sprintf(buffa,  "+%s ", opt_in[i]);
	strcat(info->def, buffa);
	G_free(opt_in[i]);
    }

    return 1;
}

#ifndef HAVE_PROJ_H
/* GPJ_get_equivalent_latlong(): only available with PROJ 4 API
 * with the new PROJ 5+ API, use pjold directly with PJ_FWD/PJ_INV transformation
*/
/**
 * \brief Define a latitude / longitude co-ordinate system with the same
 *        ellipsoid and datum parameters as an existing projected system
 *
 * This function is useful when projected co-ordinates need to be simply
 * converted to and from latitude / longitude.
 *
 * \param pjnew Pointer to pj_info struct for geographic co-ordinate system
 *        that will be created
 * \param pjold Pointer to pj_info struct for existing projected co-ordinate
 *        system
 *
 * \return 1 on success; -1 if there was an error (i.e. if the PROJ.4
 *         pj_latlong_from_proj() function returned NULL)
 **/

int GPJ_get_equivalent_latlong(struct pj_info *pjnew, struct pj_info *pjold)
{
    char *deftmp;

    pjnew->meters = 1.;
    pjnew->zone = 0;
    pjnew->def = NULL;
    sprintf(pjnew->proj, "ll");
    if ((pjnew->pj = pj_latlong_from_proj(pjold->pj)) == NULL)
	return -1;

    deftmp = pj_get_def(pjnew->pj, 1);
    pjnew->def = G_store(deftmp);
    pj_dalloc(deftmp);

    return 1;
}
#endif

/* set_proj_share()
 * 'finder function' for use with PROJ.4 pj_set_finder() function
 * this is used to find grids, usually in /usr/share/proj
 * GRASS no longer provides copies of proj grids in GRIDDIR
 * -> do not use gisbase/GRIDDIR */

const char *set_proj_share(const char *name)
{
    static char *buf = NULL;
    const char *projshare;
    static size_t buf_len = 0;
    size_t len;

    projshare = getenv("GRASS_PROJSHARE");
    if (!projshare)
	return NULL;

    len = strlen(projshare) + strlen(name) + 2;

    if (buf_len < len) {
	if (buf != NULL)
	    G_free(buf);
	buf_len = len + 20;
	buf = G_malloc(buf_len);
    }

    sprintf(buf, "%s/%s", projshare, name);

    return buf;
}

/**
 * \brief Print projection parameters as used by PROJ.4 for input and
 *        output co-ordinate systems
 *
 * \param iproj 'Input' co-ordinate system
 * \param oproj 'Output' co-ordinate system
 *
 * \return 1 on success, -1 on error (i.e. if the PROJ-style definition
 *         is NULL for either co-ordinate system)
 **/

int pj_print_proj_params(const struct pj_info *iproj, const struct pj_info *oproj)
{
    char *str;

    if (iproj) {
	str = iproj->def;
	if (str != NULL) {
	    fprintf(stderr, "%s: %s\n", _("Input Projection Parameters"),
		    str);
	    fprintf(stderr, "%s: %.16g\n", _("Input Unit Factor"),
		    iproj->meters);
	}
	else
	    return -1;
    }

    if (oproj) {
	str = oproj->def;
	if (str != NULL) {
	    fprintf(stderr, "%s: %s\n", _("Output Projection Parameters"),
		    str);
	    fprintf(stderr, "%s: %.16g\n", _("Output Unit Factor"),
		    oproj->meters);
	}
	else
	    return -1;
    }

    return 1;
}
