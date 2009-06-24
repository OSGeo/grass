
/***************************************************************************
*
* MODULE: 	g.version
* AUTHOR(S):	Michael Shapiro, CERL
*               Andreas Lange - <andreas.lange rhein-main.de>
*  	    	Justin Hickey - Thailand - jhickey hpcc.nectec.or.th
* PURPOSE: 	Output GRASS version number, date and copyright message.
*             
* COPYRIGHT:  	(C) 2000-2009 by the GRASS Development Team
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

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Flag *copyright, *build, *gish_rev;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    module->description = _("Displays version and copyright information.");

    copyright = G_define_flag();
    copyright->key = 'c';
    copyright->description = _("Print the copyright message");

    build = G_define_flag();
    build->key = 'b';
    build->description = _("Print the GRASS build information");

    gish_rev = G_define_flag();
    gish_rev->key = 'r';
    gish_rev->description =
	_("Print the GIS library revision number and time");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    fprintf(stdout, "GRASS %s (%s) %s\n",
	    GRASS_VERSION_NUMBER, GRASS_VERSION_DATE,
	    GRASS_VERSION_UPDATE_PKG);

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
	fprintf(stdout, "%s\n%s\n", rev_ver[1], rev_time[1]);
	G_free_tokens(rev_ver);
	G_free_tokens(rev_time);
    }

    return (EXIT_SUCCESS);
}
