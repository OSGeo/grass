
/***************************************************************************
*
* MODULE: 	g.version
* AUTHOR(S):	Michael Shapiro, CERL
*               Andreas Lange - <andreas.lange rhein-main.de>
*  	    	Justin Hickey - Thailand - jhickey hpcc.nectec.or.th
* PURPOSE: 	Output GRASS version number, date and copyright message.
*             
* COPYRIGHT:  	(C) 2000-2011 by the GRASS Development Team
*
*   	    	This program is free software under the GPL (>=v2)
*   	    	Read the file COPYING that comes with GRASS for details.
*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

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
    struct Flag *copyright, *build, *gish_rev, *shell;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("version"));
    module->description = _("Displays version and copyright information.");

    copyright = G_define_flag();
    copyright->key = 'c';
    copyright->description = _("Print also the copyright message");

    build = G_define_flag();
    build->key = 'b';
    build->description = _("Print also the GRASS build information");

    gish_rev = G_define_flag();
    gish_rev->key = 'r';
    gish_rev->description =
	_("Print also the GIS library revision number and time");

    shell = G_define_flag();
    shell->key = 'g';
    shell->description = _("Print info in shell script style");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (shell->answer) {
	fprintf(stdout, "version=%s\n", GRASS_VERSION_NUMBER);
	fprintf(stdout, "revision=%s\n", GRASS_VERSION_SVN);
	fprintf(stdout, "date=%s\n", GRASS_VERSION_DATE);
    }
    else {
	fprintf(stdout, "GRASS %s%s (%s)\n",
		GRASS_VERSION_NUMBER, GRASS_VERSION_SVN,
		GRASS_VERSION_DATE);
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

    return (EXIT_SUCCESS);
}
