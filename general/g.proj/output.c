/*  
 ****************************************************************************
 *
 * MODULE:       g.proj 
 * AUTHOR(S):    Paul Kelly - paul-grass@stjohnspoint.co.uk
 * PURPOSE:      Provides a means of reporting the contents of GRASS
 *               projection information files and creating
 *               new projection information files.
 * COPYRIGHT:    (C) 2003-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include <grass/config.h>

#include "local_proto.h"

static int check_xy(int shell);

/* print projection information gathered from one of the possible inputs */
void print_projinfo(int shell)
{
    int i;

    if (check_xy(shell))
	return;

    if (!shell)
	fprintf(stdout,
		"-PROJ_INFO-------------------------------------------------\n");
    for (i = 0; i < projinfo->nitems; i++) {
	if (strcmp(projinfo->key[i], "init") == 0)
	    continue;
	if (shell)
	    fprintf(stdout, "%s=%s\n", projinfo->key[i], projinfo->value[i]);
	else
	    fprintf(stdout, "%-11s: %s\n", projinfo->key[i], projinfo->value[i]);
    }

    if (projepsg) {
        const char *epsg_value, *epsg_key;

	epsg_key = projepsg->key[0];
	epsg_value = projepsg->value[0];

	if (!shell) {
	    fprintf(stdout,
		"-PROJ_EPSG-------------------------------------------------\n");
	    fprintf(stdout, "%-11s: %s\n", epsg_key, epsg_value);
	}
	else
	    fprintf(stdout, "%s=%s\n", epsg_key, epsg_value);
    }
 
    if (projunits) {
	if (!shell)
	    fprintf(stdout,
		    "-PROJ_UNITS------------------------------------------------\n");
	for (i = 0; i < projunits->nitems; i++) {
	    if (shell)
		fprintf(stdout, "%s=%s\n",
			projunits->key[i], projunits->value[i]);
	    else
		fprintf(stdout, "%-11s: %s\n",
			projunits->key[i], projunits->value[i]);
	}
    }
    
    return;
}

void print_datuminfo(void)
{
    char *datum, *params;
    struct gpj_datum dstruct;
    int validdatum = 0;

    if (check_xy(FALSE))
	return;

    GPJ__get_datum_params(projinfo, &datum, &params);

    if (datum)
	validdatum = GPJ_get_datum_by_name(datum, &dstruct);

    if (validdatum > 0)
	fprintf(stdout, "GRASS datum code: %s\nWKT Name: %s\n",
		dstruct.name, dstruct.longname);
    else if (datum)
	fprintf(stdout, "Invalid datum code: %s\n", datum);
    else
	fprintf(stdout, "Datum name not present\n");

    if (params)
	fprintf(stdout,
		"Datum transformation parameters (PROJ.4 format):\n"
		"\t%s\n", params);
    else if (validdatum > 0) {
	char *defparams;

	GPJ_get_default_datum_params_by_name(dstruct.name, &defparams);
	fprintf(stdout,
		"Datum parameters not present; default for %s is:\n"
		"\t%s\n", dstruct.name, defparams);
	G_free(defparams);
    }
    else
	fprintf(stdout, "Datum parameters not present\n");

    if (validdatum > 0)
	GPJ_free_datum(&dstruct);

    return;
}

void print_proj4(int dontprettify)
{
    struct pj_info pjinfo;
    char *proj4, *proj4mod, *i;
    const char *unfact;

    if (check_xy(FALSE))
	return;

    if (pj_get_kv(&pjinfo, projinfo, projunits) == -1)
        G_fatal_error(_("Unable to convert projection information to PROJ format"));
    proj4 = pjinfo.def;
#ifdef HAVE_PROJ_H
    proj_destroy(pjinfo.pj);
#else
    pj_free(pjinfo.pj);
#endif
    /* GRASS-style PROJ.4 strings don't include a unit factor as this is
     * handled separately in GRASS - must include it here though */
    unfact = G_find_key_value("meters", projunits);
    if (unfact != NULL && (strcmp(pjinfo.proj, "ll") != 0))
	G_asprintf(&proj4mod, "%s +to_meter=%s", proj4, unfact);
    else
	proj4mod = G_store(proj4);

    for (i = proj4mod; *i; i++) {
	/* Don't print the first space */
	if (i == proj4mod && *i == ' ')
	    continue;

	if (*i == ' ' && *(i+1) == '+' && !(dontprettify))
	    fputc('\n', stdout);
	else
	    fputc(*i, stdout);
    }
    fputc('\n', stdout);
    G_free(proj4mod);

    return;
}

#ifdef HAVE_OGR
void print_wkt(int esristyle, int dontprettify)
{
    char *outwkt;

    if (check_xy(FALSE))
	return;

    outwkt = GPJ_grass_to_wkt2(projinfo, projunits, projepsg, esristyle,
			      !(dontprettify));
    if (outwkt != NULL) {
	fprintf(stdout, "%s\n", outwkt);
	G_free(outwkt);
    }
    else
	G_warning(_("Unable to convert to WKT"));

    return;
}
#endif

static int check_xy(int shell)
{
    if (cellhd.proj == PROJECTION_XY) {
	if (shell)
	    fprintf(stdout, "name=xy_location_unprojected\n");
	else
	    fprintf(stdout, "XY location (unprojected)\n");
	return 1;
    }
    else
	return 0;
}
