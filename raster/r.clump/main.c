/****************************************************************************
 *
 * MODULE:       r.clump
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Recategorizes data in a raster map layer by grouping cells
 *		 that form physically discrete areas into unique categories.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include "local_proto.h"
#include <grass/glocale.h>


int 
main (int argc, char *argv[])
{
    struct Colors colr;
    struct Range range;
    CELL min, max;
    char *mapset;
    int in_fd, out_fd;
    char title[512];
    char name[GNAME_MAX];
    char *OUTPUT;
    char *INPUT;
    struct GModule *module;
    struct Option *opt1 ;
    struct Option *opt2 ;
    struct Option *opt3 ;

    /* please, remove before GRASS 7 released */
    struct Flag *q_flag ;

    G_gisinit (argv[0]);

/* Define the different options */

	module = G_define_module();
	module->keywords = _("raster");
    module->description =
		_("Recategorizes data in a raster map layer by grouping cells " 
		"that form physically discrete areas into unique categories.");
						
    opt1 = G_define_standard_option(G_OPT_R_INPUT);

    opt2 = G_define_standard_option(G_OPT_R_OUTPUT);

    opt3 = G_define_option() ;
    opt3->key        = "title";
    opt3->key_desc   = "\"string\"";
    opt3->type       = TYPE_STRING;
    opt3->required   = NO;
    opt3->description= _("Title, in quotes");

    /* please, remove before GRASS 7 released */
    q_flag = G_define_flag() ;
    q_flag->key         = 'q' ;  
    q_flag->description = _("Run quietly") ;

    /* parse options */
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* please, remove before GRASS 7 released */
    if(q_flag->answer) {
        putenv("GRASS_VERBOSE=0");
        G_warning(_("The '-q' flag is superseded and will be removed "
            "in future. Please use '--quiet' instead."));
    }

    INPUT  = opt1->answer;
    OUTPUT = opt2->answer;
 
    strcpy (name, INPUT);
    mapset = G_find_cell2 (name,"");
    if (!mapset)
        G_fatal_error (_("Raster map <%s> not found"), INPUT);

    if (G_legal_filename (OUTPUT) < 0)
    	G_fatal_error (_("<%s> is an illegal file name"), OUTPUT);

    in_fd = G_open_cell_old (name, mapset);
    if (in_fd < 0)
        G_fatal_error (_("Unable to open raster map <%s>"), INPUT);

    out_fd = G_open_cell_new (OUTPUT);
    if (out_fd < 0)
        G_fatal_error (_("Unable to create raster map <%s>"), OUTPUT);

    clump (in_fd, out_fd);

    G_message (_("Creating support files..."));
  
    G_close_cell (in_fd);
    G_close_cell (out_fd);


/* build title */
    if (opt3->answer != NULL)
	strcpy (title, opt3->answer);
    else
        sprintf (title, "clump of %s in %s", name, mapset);

    G_put_cell_title (OUTPUT, title);
    G_read_range (OUTPUT, G_mapset(), &range);
    G_get_range_min_max(&range, &min, &max);
    G_make_random_colors (&colr, min, max);
    G_write_colors (OUTPUT, G_mapset(), &colr);

    G_message (_("%d clumps"), range.max);
    exit(EXIT_SUCCESS);
}
