
/***************************************************************************
*
* MODULE: 	g.version
* AUTHOR(S):	Michael Shapiro, CERL
*               Andreas Lange - <andreas.lange rhein-main.de>
*  	    	Justin Hickey - Thailand - jhickey hpcc.nectec.or.th
*               Extended info by Martin Landa <landa.martin gmail.com>
* PURPOSE: 	Output GRASS version number, date and copyright message.
*             
* COPYRIGHT:  	(C) 2000-2013 by the GRASS Development Team
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

#include <proj_api.h>

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

static const char COPYING[] =
#include <grass/copying.h>
;

static const char GRASS_CONFIGURE_PARAMS[] =
#include <grass/confparms.h>
;

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Flag *copyright, *build, *gish_rev, *shell, *extended;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("supporting"));
    module->label = _("Displays GRASS version info.");
    module->description = _("Optionally also print build or copyright information.");

    copyright = G_define_flag();
    copyright->key = 'c';
    copyright->description = _("Print also the copyright message");
    copyright->guisection = _("Additional info");

    build = G_define_flag();
    build->key = 'b';
    build->description = _("Print also the build information");
    build->guisection = _("Additional info");

    gish_rev = G_define_flag();
    gish_rev->key = 'r';
    gish_rev->description =
	_("Print also the GIS library revision number and date");
    gish_rev->guisection = _("Additional info");

    extended = G_define_flag();
    extended->key = 'e';
    extended->label = _("Print also extended info for additional libraries");
    extended->description = _("GDAL/OGR, PROJ.4, GEOS");
    extended->guisection = _("Additional info");

    shell = G_define_flag();
    shell->key = 'g';
    shell->description = _("Print info in shell script style (including SVN revision number)");
    shell->guisection = _("Shell");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (shell->answer) {
	fprintf(stdout, "version=%s\n", GRASS_VERSION_NUMBER);
	fprintf(stdout, "date=%s\n", GRASS_VERSION_DATE);
	fprintf(stdout, "revision=%s\n", GRASS_VERSION_SVN);
	fprintf(stdout, "build_date=%d-%02d-%02d\n", YEAR, MONTH, DAY);
    }
    else {
	fprintf(stdout, "GRASS %s (%s)\n",
		GRASS_VERSION_NUMBER, GRASS_VERSION_DATE);
    }
    
    if (copyright->answer) {
	fprintf(stdout, "\n");
	fputs(COPYING, stdout);
    }

    if (build->answer) {
	fprintf(stdout, "\n");
	fputs(GRASS_CONFIGURE_PARAMS, stdout);
	fprintf(stdout, "\n");
    }

    if (gish_rev->answer) {
	/* fprintf(stdout, "%s\n%s\n", GIS_H_VERSION, GIS_H_DATE); */
	char **rev_ver = G_tokenize(GIS_H_VERSION, "$");
	char **rev_time = G_tokenize(GIS_H_DATE, "$");
	if (shell->answer) {
	    fprintf(stdout, "libgis_revision=%s\n", strstr(rev_ver[1], " ") + 1);
	    fprintf(stdout, "libgis_date=\"%s\"\n", strstr(rev_time[1], " ") + 1);
	}
	else {
	    fprintf(stdout, "libgis %s\nlibgis %s\n", rev_ver[1], rev_time[1]);
	}
	G_free_tokens(rev_ver);
	G_free_tokens(rev_time);
    }

    if (extended->answer) {
        char *proj = NULL;
        G_asprintf(&proj, "%d", PJ_VERSION);
        if (strlen(proj) == 3) {
            if (shell->answer)
                fprintf(stdout, "proj4=%c.%c.%c\n", proj[0], proj[1], proj[2]); 
            else
                fprintf(stdout, "PROJ.4: %c.%c.%c\n", proj[0], proj[1], proj[2]); 
        }
        else {
            if (shell->answer)
                fprintf(stdout, "proj4=%s\n", proj);
            else
                fprintf(stdout, "PROJ.4: %s\n", proj);
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
