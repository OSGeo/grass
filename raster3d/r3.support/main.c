/*
 **********************************************************************
 *
 * MODULE:       r3.support (GRASS core)
 *
 * AUTHOR(S):    Soeren Gebbert vTI/AK based on r.support
 *               Original r.support by Michael Shapiro - CERL
 *
 * PURPOSE:      Build support files for raster map
 *               - Edit header, units
 *               - Update status (range)
 *
 * COPYRIGHT:    (C) 2000-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General 
 *               Public License (>=v2). Read the file COPYING that comes
 *               with GRASS for details.
 *
 **********************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>
#include "local_proto.h"

#define MAX_TITLE_LEN 1022

int main(int argc, char *argv[])
{
    const char *mapset;
    struct GModule *module;
    struct Option *raster, *title_opt, *history_opt;
    struct Option *datasrc1_opt, *datasrc2_opt, *datadesc_opt;
    struct Option *map_opt, *units_opt, *vunits_opt;
    struct Option *load_opt, *save_opt;
    struct Flag *stats_flag;
    const char *infile;
    char title[MAX_TITLE_LEN + 1];
    struct History hist;

    /* Initialize GIS engine */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("metadata"));
    module->description = _("Allows creation and/or modification of "
			    "raster3d map layer support files.");

    raster = G_define_standard_option(G_OPT_R3_MAP);

    title_opt = G_define_option();
    title_opt->key = "title";
    title_opt->key_desc = "phrase";
    title_opt->type = TYPE_STRING;
    title_opt->required = NO;
    title_opt->description = _("Text to use for map title");

    history_opt = G_define_option();
    history_opt->key = "history";
    history_opt->key_desc = "phrase";
    history_opt->type = TYPE_STRING;
    history_opt->required = NO;
    history_opt->description =
	_("Text to append to the next line of the map's metadata file");

    units_opt = G_define_option();
    units_opt->key = "unit";
    units_opt->type = TYPE_STRING;
    units_opt->required = NO;
    units_opt->description = _("The map data unit");

    vunits_opt = G_define_option();
    vunits_opt->key = "vunit";
    vunits_opt->type = TYPE_STRING;
    vunits_opt->required = NO;
    vunits_opt->description = _("The vertical unit of the map");
    
    datasrc1_opt = G_define_option();
    datasrc1_opt->key = "source1";
    datasrc1_opt->key_desc = "phrase";
    datasrc1_opt->type = TYPE_STRING;
    datasrc1_opt->required = NO;
    datasrc1_opt->description = _("Text to use for data source, line 1");

    datasrc2_opt = G_define_option();
    datasrc2_opt->key = "source2";
    datasrc2_opt->key_desc = "phrase";
    datasrc2_opt->type = TYPE_STRING;
    datasrc2_opt->required = NO;
    datasrc2_opt->description = _("Text to use for data source, line 2");

    datadesc_opt = G_define_option();
    datadesc_opt->key = "description";
    datadesc_opt->key_desc = "phrase";
    datadesc_opt->type = TYPE_STRING;
    datadesc_opt->required = NO;
    datadesc_opt->description =
	_("Text to use for data description or keyword(s)");

    map_opt = G_define_option();
    map_opt->key = "raster";
    map_opt->type = TYPE_STRING;
    map_opt->required = NO;
    map_opt->gisprompt = "old,cell,raster";
    map_opt->description = _("Raster map from which to copy category table");

    load_opt = G_define_standard_option(G_OPT_F_INPUT);
    load_opt->key = "loadhistory";
    load_opt->required = NO;
    load_opt->description = _("Text file from which to load history");

    save_opt = G_define_standard_option(G_OPT_F_OUTPUT);
    save_opt->key = "savehistory";
    save_opt->required = NO;
    save_opt->description = _("Text file in which to save history");

    stats_flag = G_define_flag();
    stats_flag->key = 's';
    stats_flag->description = _("Update range");

    /* Parse command-line options */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Make sure raster exists and set mapset */
    infile = raster->answer;
    mapset = G_find_raster3d(infile, G_mapset());	/* current mapset only for editing */
    if (!mapset || strcmp(mapset, G_mapset()) != 0)
	G_fatal_error(_("3D raster map <%s> not found"), infile);

    if (title_opt->answer) {
	strncpy(title, title_opt->answer, MAX_TITLE_LEN);
	title[MAX_TITLE_LEN - 1] = '\0';	/* strncpy doesn't null terminate over-sized input */
	G_strip(title);
	G_debug(3, "map title= [%s]  (%d chars)", title, (int)strlen(title));
        
        Rast3d_read_history(raster->answer, "", &hist);
        Rast_set_history(&hist, HIST_TITLE, title);
	Rast3d_write_history(raster->answer, &hist);
    }

    if (save_opt->answer) {
	FILE *fp = fopen(save_opt->answer, "w");
	int i;

	if (!fp)
	    G_fatal_error(_("Unable to open output file <%s>"), save_opt->answer);

	Rast3d_read_history(raster->answer, "", &hist);

	for (i = 0; i < Rast_history_length(&hist); i++)
	    fprintf(fp, "%s\n", Rast_history_line(&hist, i));

	fclose(fp);
    }

    if (load_opt->answer) {
	FILE *fp = fopen(load_opt->answer, "r");

	if (!fp)
	    G_fatal_error(_("Unable to open input file <%s>"), load_opt->answer);

	Rast3d_read_history(raster->answer, "", &hist);

	Rast_clear_history(&hist);

	for (;;) {
	    char buf[80];
	    if (!G_getl2(buf, sizeof(buf), fp))
		break;
	    Rast_append_history(&hist, buf);
	}

	fclose(fp);

	Rast3d_write_history(raster->answer, &hist);
    }

    if (history_opt->answer) {
	Rast3d_read_history(raster->answer, "", &hist);

	/* two less than defined as if only one less a newline gets appended in the hist file. bug? */
	/* Should be RECORD_LEN, but r.info truncates at > 71 chars */
	if (strlen(history_opt->answer) > 71) {
	    int i;

	    for (i = 0; i < strlen(history_opt->answer); i += 71) {
		char buf[72];

		strncpy(buf, &history_opt->answer[i], sizeof(buf)-1);
		buf[sizeof(buf)-1] = '\0';

		Rast_append_history(&hist, buf);
	    }
	}
	else
	    Rast_append_history(&hist, history_opt->answer);

	Rast3d_write_history(raster->answer, &hist);
    }
    
    if(units_opt->answer || vunits_opt->answer) {
        RASTER3D_Map *map;
        
        map = Rast3d_open_cell_old(raster->answer, G_mapset(), \
                RASTER3D_DEFAULT_WINDOW, RASTER3D_TILE_SAME_AS_FILE, RASTER3D_USE_CACHE_DEFAULT);
        
        /* Modify the units */
        if (units_opt->answer)
            Rast3d_set_unit(map, units_opt->answer);

        if (vunits_opt->answer)
            Rast3d_set_vertical_unit(map, vunits_opt->answer);
        
        Rast3d_rewrite_header(map);
        Rast3d_close(map);
    
    }

    if (datasrc1_opt->answer || datasrc2_opt->answer || datadesc_opt->answer) {
	Rast3d_read_history(raster->answer, "", &hist);

	if (datasrc1_opt->answer)
	    Rast_set_history(&hist, HIST_DATSRC_1, datasrc1_opt->answer);

	if (datasrc2_opt->answer)
	    Rast_set_history(&hist, HIST_DATSRC_2, datasrc2_opt->answer);

	if (datadesc_opt->answer)
	    Rast_set_history(&hist, HIST_KEYWRD, datadesc_opt->answer);

	Rast3d_write_history(raster->answer, &hist);
    }

    if (map_opt->answer) {	/* use cats from another map */
	int fd;
	struct Categories cats;

	fd = Rast_open_old(infile, "");
	Rast_init_cats("", &cats);
	if (Rast3d_read_cats(map_opt->answer, "", &cats) < 0)
	    G_fatal_error(_("Unable to read category file of raster map <%s>"),
			  map_opt->answer);

	Rast3d_write_cats(infile, &cats);
	G_message(_("cats table for [%s] set to %s"),
		  infile, map_opt->answer);
	Rast_close(fd);
	Rast_free_cats(&cats);
    }

    /* Check the histogram and range */
    if (stats_flag->answer)
	check_stats(raster->answer);

    return EXIT_SUCCESS;
}
