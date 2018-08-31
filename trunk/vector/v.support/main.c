
/****************************************************************
 *
 * MODULE:     v.support
 *
 * AUTHOR(S):  Markus Neteler
 *
 * PURPOSE:    updates metadata of vector map
 *
 * COPYRIGHT:  (C) 2007, 2017 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct Map_info Map;
    struct GModule *module;
    struct Option *map, *organization, *date, *person, *map_name, *map_date,
	*scale, *comment, *zone, *thresh, *cmdhist;
    struct Flag *r_flag, *h_flag;

    /* initialize GIS environment */
    G_gisinit(argv[0]);

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("metadata"));
    module->description = _("Updates vector map metadata.");

    /* Define the different options as defined in gis.h */
    map = G_define_standard_option(G_OPT_V_MAP);

    organization = G_define_option();
    organization->key = "organization";
    organization->key_desc = "phrase";
    organization->type = TYPE_STRING;
    organization->required = NO;
    organization->description =
	_("Organization where vector map was created");

    /* don't predefine answers to not overwrite existing information */
    date = G_define_option();
    date->key = "date";
    date->key_desc = "datestring";
    date->type = TYPE_STRING;
    date->required = NO;
    date->description =
	_("Date of vector map digitization (e.g., \"15 Mar 2007\")");

    person = G_define_option();
    person->key = "person";
    person->key_desc = "phrase";
    person->type = TYPE_STRING;
    person->required = NO;
    person->description = _("Person who created vector map");

    map_name = G_define_option();
    map_name->key = "map_name";
    map_name->key_desc = "phrase";
    map_name->type = TYPE_STRING;
    map_name->required = NO;
    map_name->description = _("Vector map title");

    map_date = G_define_option();
    map_date->key = "map_date";
    map_date->key_desc = "datestring";
    map_date->type = TYPE_STRING;
    map_date->required = NO;
    map_date->description =
	_("Date when the source map was originally produced");

    scale = G_define_option();
    scale->key = "scale";
    scale->type = TYPE_INTEGER;
    scale->required = NO;
    scale->description = _("Vector map scale number (e.g., 24000)");

    zone = G_define_option();
    zone->key = "zone";
    zone->type = TYPE_INTEGER;
    zone->required = NO;
    zone->description = _("Vector map projection zone");

    thresh = G_define_option();
    thresh->key = "threshold";
    thresh->type = TYPE_DOUBLE;
    thresh->required = NO;
    thresh->description =
	_("Vector map digitizing threshold number (e.g., 0.5)");

    comment = G_define_option();
    comment->key = "comment";
    comment->key_desc = "phrase";
    comment->type = TYPE_STRING;
    comment->required = NO;
    comment->description =
	_("Text to append to the comment line of the map's metadata file");

    cmdhist = G_define_option();
    cmdhist->key = "cmdhist";
    cmdhist->key_desc = "command";
    cmdhist->type = TYPE_STRING;
    cmdhist->required = NO;
    cmdhist->description =
	_("Command line to store into vector map history file (used for vector scripts)");

    r_flag = G_define_flag();
    r_flag->key = 'r';
    r_flag->description = _("Replace comment instead of appending it");

    h_flag = G_define_flag();
    h_flag->key = 'h';
    h_flag->description = _("Replace command line instead of appending it");

    /* options and flags parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Vect_set_open_level(1);
    if (Vect_open_old(&Map, map->answer, "") < 1)
	G_fatal_error(_("Unable to open vector map <%s>"), map->answer);

    /* modify 'head' file */
    Vect_read_header(&Map);

    if (organization->answer)
	Vect_set_organization(&Map, organization->answer);
    if (date->answer)
	Vect_set_date(&Map, date->answer);
    if (person->answer)
	Vect_set_person(&Map, person->answer);
    if (map_name->answer)
	Vect_set_map_name(&Map, map_name->answer);
    if (map_date->answer)
	Vect_set_map_date(&Map, map_date->answer);

    if (scale->answer) {
	int scalenum = atoi(scale->answer);

	if (scalenum == 0)
	    scalenum = 1;
	Vect_set_scale(&Map, scalenum);
    }

    if (zone->answer)
	Vect_set_zone(&Map, atoi(zone->answer));

    if (thresh->answer)
	Vect_set_thresh(&Map, atof(thresh->answer));

    if (comment->answer) {	/* apparently only one line comments allowed, so we use space to delimit */
	char *temp;

	if (r_flag->answer || strlen(Vect_get_comment(&Map)) == 0) {	/* check if new/replacing or adding */
	    G_asprintf(&temp, "%s", comment->answer);
	}
	else {
	    G_asprintf(&temp, "%s %s", Vect_get_comment(&Map), comment->answer);
	}
	Vect_set_comment(&Map, temp);
    }

    Vect_write_header(&Map);


    /* modify 'hist' file */
    if (cmdhist->answer) {
	char buf[2000];		/* derived from Vect_hist_command() */

	/* Open history file for modification */
	sprintf(buf, "%s/%s", GV_DIRECTORY, Map.name);
	if (h_flag->answer)
	    Map.hist_fp = G_fopen_new(buf, GV_HIST_ELEMENT);
	else
	    Map.hist_fp = G_fopen_modify(buf, GV_HIST_ELEMENT);
	if (Map.hist_fp == NULL) {
	    G_warning(_("Unable to open history file for vector map <%s>"),
		      Vect_get_full_name(&Map));
	    Vect_close(&Map);
	    exit(EXIT_FAILURE);
	}
	if (!h_flag->answer) {
	    G_fseek(Map.hist_fp, (long)0, SEEK_END);
	    Vect_hist_write(&Map,
			    "---------------------------------------------------------------------------------\n");
	}
	Vect_hist_write(&Map, "COMMAND: ");
	Vect_hist_write(&Map, cmdhist->answer);
	Vect_hist_write(&Map, "\n");
	sprintf(buf, "GISDBASE: %s\n", G_gisdbase());
	Vect_hist_write(&Map, buf);
	sprintf(buf, "LOCATION: %s MAPSET: %s USER: %s DATE: %s\n",
		G_location(), G_mapset(), G_whoami(), G_date());
	Vect_hist_write(&Map, buf);
    }

    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}
