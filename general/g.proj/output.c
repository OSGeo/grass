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
#include <string.h>
#include <errno.h>

#include <proj_api.h>

#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include <grass/config.h>

#include "local_proto.h"

static int check_xy(int shell);

void print_projinfo(int shell)
{
    int i;

    if (check_xy(shell))
	return;

    if (!shell)
	fprintf(stdout,
		"-PROJ_INFO-------------------------------------------------\n");
    for (i = 0; i < projinfo->nitems; i++) {
	if (shell)
	    fprintf(stdout, "%s=%s\n", projinfo->key[i], projinfo->value[i]);
	else
	    fprintf(stdout, "%-11s: %s\n", projinfo->key[i], projinfo->value[i]);
    }
    
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

    pj_get_kv(&pjinfo, projinfo, projunits);
    proj4 = pj_get_def(pjinfo.pj, 0);
    pj_free(pjinfo.pj);

    /* GRASS-style PROJ.4 strings don't include a unit factor as this is
     * handled separately in GRASS - must include it here though */
    unfact = G_find_key_value("meters", projunits);
    if (unfact != NULL && (strcmp(pjinfo.proj, "ll") != 0))
	G_asprintf(&proj4mod, "%s +to_meter=%s", proj4, unfact);
    else
	proj4mod = G_store(proj4);
    pj_dalloc(proj4);

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

    outwkt = GPJ_grass_to_wkt(projinfo, projunits, esristyle,
			      !(dontprettify));
    if (outwkt != NULL) {
	fprintf(stdout, "%s\n", outwkt);
	G_free(outwkt);
    }
    else
	G_warning(_("%s: Unable to convert to WKT"), G_program_name());

    return;
}
#endif

void create_location(char *location)
{
    int ret;

    if (location) {
	ret = G__make_location(location, &cellhd, projinfo, projunits, NULL);
	if (ret == 0)
	    G_message(_("Location <%s> created"), location);
	else if (ret == -1)
	    G_fatal_error(_("Unable to create location <%s>: %s"),
			  location, strerror(errno));
	else if (ret == -2)
			 G_fatal_error(_("Unable to create projection files: %s"),
			  strerror(errno));
	else
	    /* Shouldn't happen */
	    G_fatal_error(_("Unspecified error while creating new location"));
    }
    else {
	/* Create flag given but no location specified; overwrite
	 * projection files for current location */

	const char *mapset = G_mapset();
	struct Cell_head old_cellhd;

	if (strcmp(mapset, "PERMANENT") != 0)
	    G_fatal_error(_("You must select the PERMANENT mapset before updating the "
			   "current location's projection. (Current mapset is %s)"),
			  mapset);

	/* Read projection information from current location first */
	G_get_default_window(&old_cellhd);
	
	char path[GPATH_MAX];
	
	/* Write out the PROJ_INFO, and PROJ_UNITS if available. */
	if (projinfo != NULL) {
	    G_file_name(path, "", "PROJ_INFO", "PERMANENT");
	    G_write_key_value_file(path, projinfo);
	}
	
	if (projunits != NULL) {
	    G_file_name(path, "", "PROJ_UNITS", "PERMANENT");
	    G_write_key_value_file(path, projunits);
	}
	
	if ((old_cellhd.zone != cellhd.zone) ||
	    (old_cellhd.proj != cellhd.proj)) {
	    /* Recreate the default, and current window files if projection
	     * number or zone have changed */
	    G__put_window(&cellhd, "", "DEFAULT_WIND");
		G__put_window(&cellhd, "", "WIND");
		G_message(_("N.B. The default region was updated to the new projection, but if you have "
			    "multiple mapsets g.region -d should be run in each to update the region from "
			    "the default."));
	}
	G_message(_("Projection information updated!"));
    }

    return;
}

static int check_xy(int shell)
{
    if (cellhd.proj == PROJECTION_XY) {
	if (shell)
	    fprintf(stdout, "name=");
	fprintf(stdout, "XY location (unprojected)\n");
	return 1;
    }
    else
	return 0;
}
