/***************************************************************************
*
* MODULE: 	g.version
* AUTHOR(S):	Michael Shapiro, CERL
*               Andreas Lange - <andreas.lange rhein-main.de>
*  	    	Justin Hickey - Thailand - jhickey hpcc.nectec.or.th
* PURPOSE: 	Output GRASS version number, date and copyright message.
*             
* COPYRIGHT:  	(C) 2000-2007 by the GRASS Development Team
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
    struct Flag *copyright, *build;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("general");
    module->description = _("Displays version and copyright information.");

    copyright = G_define_flag();
    copyright->key = 'c';
    copyright->description = _("Print the copyright message");

    build = G_define_flag();
    build->key = 'b';
    build->description = _("Print the GRASS build information");

    if (argc > 1 && G_parser(argc, argv))
	exit(EXIT_FAILURE);


    fprintf (stdout, "GRASS %s (%s) %s\n",
    	GRASS_VERSION_NUMBER, GRASS_VERSION_DATE, GRASS_VERSION_UPDATE_PKG );

    if (copyright->answer){
	fprintf(stdout, "\n");
    	fputs (COPYING, stdout);
    }

    if (build->answer){
	fprintf(stdout, "\n");
    	fputs (GRASS_CONFIGURE_PARAMS, stdout);
	fprintf(stdout, "\n");
    }

    return (EXIT_SUCCESS);
}
