
/***************************************************************************
*
* MODULE: 	g.version
* AUTHOR(S):	Michael Shapiro, CERL
*               Andreas Lange - <andreas.lange rhein-main.de>
*  	    	Justin Hickey - Thailand - jhickey hpcc.nectec.or.th
*               Extended info by Martin Landa <landa.martin gmail.com>
* PURPOSE: 	Output GRASS version number, date and copyright message.
*             
* COPYRIGHT:  	(C) 2000-2015 by the GRASS Development Team
*
*   	    	This program is free software under the GPL (>=v2)
*   	    	Read the file COPYING that comes with GRASS for details.
*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

#ifdef HAVE_PROJ_H
#include <proj.h>
#else
#include <proj_api.h>
#endif

#ifdef HAVE_GDAL
#include <gdal_version.h>
#endif

#ifdef HAVE_GEOS
#include <geos_c.h>
#endif

#ifdef HAVE_SQLITE
#include <sqlite3.h>
#endif

#ifndef GRASS_VERSION_UPDATE_PKG
#define GRASS_VERSION_UPDATE_PKG "0.1"
#endif

/* TODO: remove this style of include */
static const char COPYING[] =
#include <grass/copying.h>
;

static const char CITING[] =
#include <grass/citing.h>
;

static const char GRASS_CONFIGURE_PARAMS[] =
#include <grass/confparms.h>
;

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Flag *copyright, *build, *gish_rev, *cite_flag, *shell, *extended;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("support"));
    G_add_keyword(_("citing"));
    G_add_keyword(_("copyright"));
    G_add_keyword(_("version"));
    G_add_keyword(_("license"));
    module->label = _("Displays GRASS GIS version info.");
    module->description = _("Optionally also prints build or copyright information.");

    copyright = G_define_flag();
    copyright->key = 'c';
    copyright->description = _("Print also the copyright message");
    copyright->guisection = _("Additional info");

    cite_flag = G_define_flag();
    cite_flag->key = 'x';
    cite_flag->description = _("Print also the citation options");
    cite_flag->guisection = _("Additional info");

    build = G_define_flag();
    build->key = 'b';
    build->description = _("Print also the build information");
    build->guisection = _("Additional info");

    gish_rev = G_define_flag();
    gish_rev->key = 'r';
    /* this was never the library revision number and date
     * it was the revision number and date of gis.h
     * now it is the git hash and date of all GRASS headers
     * (and anything else in include) */
    gish_rev->description =
	_("Print also the GIS library revision number and date");
    gish_rev->guisection = _("Additional info");

    extended = G_define_flag();
    extended->key = 'e';
    extended->label = _("Print also extended info for additional libraries");
    extended->description = _("GDAL/OGR, PROJ, GEOS");
    extended->guisection = _("Additional info");

    shell = G_define_flag();
    shell->key = 'g';
    shell->description = _("Print info in shell script style (including Git reference commit)");
    shell->guisection = _("Shell");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (shell->answer) {
	fprintf(stdout, "version=%s\n", GRASS_VERSION_NUMBER);
	fprintf(stdout, "date=%s\n", GRASS_VERSION_DATE);
	fprintf(stdout, "revision=%s\n", GRASS_VERSION_GIT);
	fprintf(stdout, "build_date=%d-%02d-%02d\n", YEAR, MONTH, DAY);
	fprintf(stdout, "build_platform=%s\n", ARCH);
        fprintf(stdout, "build_off_t_size=%lu\n", sizeof(off_t));
    }
    else {
	fprintf(stdout, "GRASS %s (%s)\n",
		GRASS_VERSION_NUMBER, GRASS_VERSION_DATE);
    }
    
    if (copyright->answer) {
	fprintf(stdout, "\n");
	fputs(COPYING, stdout);
    }

    if (cite_flag->answer) {
	fprintf(stdout, "\n");
	fputs(CITING, stdout);
    }

    if (build->answer) {
	fprintf(stdout, "\n");
	fputs(GRASS_CONFIGURE_PARAMS, stdout);
	fprintf(stdout, "\n");
    }

    if (gish_rev->answer) {
	char *rev_ver = GIS_H_VERSION;
	char *rev_time = GIS_H_DATE;
	int no_libgis = FALSE;

	if (*rev_ver && *rev_time) {
	    if (shell->answer) {
                fprintf(stdout, "libgis_revision=%s\n", rev_ver);
                fprintf(stdout, "libgis_date=%s\n", rev_time);
	    }
	    else {
		fprintf(stdout, "libgis revision: %s\n", rev_ver);
		fprintf(stdout, "libgis date: %s\n", rev_time);
	    }
	}
	else {
	    no_libgis = TRUE;
	    if (shell->answer) {
		fprintf(stdout, "libgis_revision=\n");
		fprintf(stdout, "libgis_date=\n");
		G_warning("GRASS GIS libgis version and date number not available");
		/* this can be alternatively fatal error or it can cause
		   fatal error later */
	    }
	    else {
		fprintf(stdout,
			_("Cannot determine GRASS libgis version and date number."
			 " The GRASS build might be broken."
			 " Report this to developers or packagers.\n"));
	    }
	}
	if (no_libgis) {
	    G_debug(1,
		    _("GRASS GIS libgis version and date number don't have the expected format."
		     " Trying to print the original strings..."));
	    G_debug(1, _("GIS_H_VERSION=\"%s\""), GIS_H_VERSION);
	    G_debug(1, _("GIS_H_DATE=\"%s\""), GIS_H_DATE);
	}
    }

    if (extended->answer) {
        char *proj = NULL;
#ifdef HAVE_PROJ_H
        G_asprintf(&proj, "%d%d%d", PROJ_VERSION_MAJOR,
	                            PROJ_VERSION_MINOR,
				    PROJ_VERSION_PATCH);
#else
        G_asprintf(&proj, "%d", PJ_VERSION);
#endif
        if (strlen(proj) == 3) {
            if (shell->answer)
                fprintf(stdout, "proj=%c.%c.%c\n", proj[0], proj[1], proj[2]); 
            else
                fprintf(stdout, "PROJ: %c.%c.%c\n", proj[0], proj[1], proj[2]); 
        }
        else {
            if (shell->answer)
                fprintf(stdout, "proj=%s\n", proj);
            else
                fprintf(stdout, "PROJ: %s\n", proj);
        }
#ifdef HAVE_GDAL
        if (shell->answer)
            fprintf(stdout, "gdal=%s\n", GDAL_RELEASE_NAME);
        else
            fprintf(stdout, "GDAL/OGR: %s\n", GDAL_RELEASE_NAME);
#else
        if (shell->answer)
            fprintf(stdout, "gdal=\n");
        else
            fprintf(stdout, "%s\n", _("GRASS not compiled with GDAL/OGR support"));
#endif
#ifdef HAVE_GEOS
        if (shell->answer)
            fprintf(stdout, "geos=%s\n", GEOS_VERSION);
        else
            fprintf(stdout, "GEOS: %s\n", GEOS_VERSION);
#else
        if (shell->answer)
            fprintf(stdout, "geos=\n");
        else
            fprintf(stdout, "%s\n", _("GRASS not compiled with GEOS support"));
#endif
#ifdef HAVE_SQLITE
        if (shell->answer)
            fprintf(stdout, "sqlite=%s\n", SQLITE_VERSION);
        else
            fprintf(stdout, "SQLite: %s\n", SQLITE_VERSION);
#else
        if (shell->answer)
            fprintf(stdout, "sqlite=\n");
        else
            fprintf(stdout, "%s\n", _("GRASS not compiled with SQLite support"));
#endif
    }
    
    return (EXIT_SUCCESS);
}
