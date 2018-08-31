/*
 **********************************************************************
 *
 * MODULE:       r.support (GRASS core)
 *
 * AUTHOR(S):    Original by Michael Shapiro - CERL
 *               Preliminary parser support by Markus Neteler, rast parameter
 *               Port to 6.x by Brad Douglas
 *
 * PURPOSE:      Build support files for raster map
 *               - Edit header
 *               - Update status (histogram, range)
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
#include <grass/glocale.h>
#include "local_proto.h"

/* two less than lib/gis/put_title.c  Rast_put_cell_title()
   if only one less a newline gets appended in the cats file. bug? */
#define MAX_TITLE_LEN 1022

int main(int argc, char *argv[])
{
    char rname[GNAME_MAX];	/* Reclassed map name */
    char rmapset[GMAPSET_MAX];	/* Reclassed mapset   */
    const char *mapset;		/* Raster mapset      */
    struct Cell_head cellhd;
    struct GModule *module;
    struct Option *raster, *title_opt, *history_opt;
    struct Option *datasrc1_opt, *datasrc2_opt, *datadesc_opt;
    struct Option *map_opt, *units_opt, *vdatum_opt;
    struct Option *load_opt, *save_opt;
    struct Flag *stats_flag, *null_flag, *del_flag;
    int is_reclass;		/* Is raster reclass? */
    const char *infile;
    struct History hist;

    /* Initialize GIS engine */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("metadata"));
    module->description = _("Allows creation and/or modification of "
			    "raster map layer support files.");

    raster = G_define_standard_option(G_OPT_R_MAP);

    title_opt = G_define_option();
    title_opt->key = "title";
    title_opt->key_desc = "phrase";
    title_opt->type = TYPE_STRING;
    title_opt->required = NO;
    title_opt->description = _("Title for resultant raster map");

    history_opt = G_define_option();
    history_opt->key = "history";
    history_opt->key_desc = "phrase";
    history_opt->type = TYPE_STRING;
    history_opt->required = NO;
    history_opt->description =
	_("Text to append to the next line of the map's metadata file");

    units_opt = G_define_option();
    units_opt->key = "units";
    units_opt->type = TYPE_STRING;
    units_opt->required = NO;
    units_opt->description = _("Text to use for map data units");

    vdatum_opt = G_define_option();
    vdatum_opt->key = "vdatum";
    vdatum_opt->type = TYPE_STRING;
    vdatum_opt->required = NO;
    vdatum_opt->description = _("Text to use for map vertical datum");

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
    stats_flag->description = _("Update statistics (histogram, range)");

    null_flag = G_define_flag();
    null_flag->key = 'n';
    null_flag->description = _("Create/reset the null file");

    del_flag = G_define_flag();
    del_flag->key = 'd';
    del_flag->description = _("Delete the null file");

    /* Parse command-line options */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Make sure raster exists and set mapset */
    infile = raster->answer;
    mapset = G_find_raster2(infile, G_mapset());	/* current mapset only for editing */
    if (!mapset || strcmp(mapset, G_mapset()) != 0)
	G_fatal_error(_("Raster map <%s> not found in current mapset"), infile);

    Rast_get_cellhd(raster->answer, "", &cellhd);
    is_reclass = (Rast_is_reclass(raster->answer, "", rname, rmapset) > 0);

    if (title_opt->answer) {
        Rast_put_cell_title(raster->answer, title_opt->answer);
	G_debug(3, "map title= [%s]  (%li chars)", title_opt->answer, strlen(title_opt->answer));
    }

    if (save_opt->answer) {
	FILE *fp = fopen(save_opt->answer, "w");
	int i;

	if (!fp)
	    G_fatal_error(_("Unable to open output file <%s>"), save_opt->answer);

	Rast_read_history(raster->answer, "", &hist);

	for (i = 0; i < Rast_history_length(&hist); i++)
	    fprintf(fp, "%s\n", Rast_history_line(&hist, i));

	fclose(fp);
    }

    if (load_opt->answer) {
	FILE *fp = fopen(load_opt->answer, "r");

	if (!fp)
	    G_fatal_error(_("Unable to open input file <%s>"), load_opt->answer);

	Rast_read_history(raster->answer, "", &hist);

	Rast_clear_history(&hist);

	for (;;) {
	    char buf[80];
	    if (!G_getl2(buf, sizeof(buf), fp))
		break;
	    Rast_append_history(&hist, buf);
	}

	fclose(fp);

	Rast_write_history(raster->answer, &hist);
    }

    if (history_opt->answer) {
	Rast_read_history(raster->answer, "", &hist);

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

	Rast_write_history(raster->answer, &hist);
    }

    if (units_opt->answer)
	Rast_write_units(raster->answer, units_opt->answer);

    if (vdatum_opt->answer)
	Rast_write_vdatum(raster->answer, vdatum_opt->answer);

    if (datasrc1_opt->answer || datasrc2_opt->answer || datadesc_opt->answer) {
	Rast_read_history(raster->answer, "", &hist);

	if (datasrc1_opt->answer)
	    Rast_set_history(&hist, HIST_DATSRC_1, datasrc1_opt->answer);

	if (datasrc2_opt->answer)
	    Rast_set_history(&hist, HIST_DATSRC_2, datasrc2_opt->answer);

	if (datadesc_opt->answer)
	    Rast_set_history(&hist, HIST_KEYWRD, datadesc_opt->answer);

	Rast_write_history(raster->answer, &hist);
    }

    if (map_opt->answer) {	/* use cats from another map */
	int fd;
	struct Categories cats;

	fd = Rast_open_old(infile, "");
	Rast_init_cats("", &cats);
	if (Rast_read_cats(map_opt->answer, "", &cats) < 0)
	    G_fatal_error(_("Unable to read category file of raster map <%s>"),
			  map_opt->answer);

	Rast_write_cats(infile, &cats);
	G_message(_("cats table for [%s] set to %s"),
		  infile, map_opt->answer);
	Rast_close(fd);
	Rast_free_cats(&cats);
    }


    if (title_opt->answer || history_opt->answer || units_opt->answer
	|| vdatum_opt->answer || datasrc1_opt->answer || datasrc2_opt->answer
	|| datadesc_opt->answer || map_opt->answer)
	exit(EXIT_SUCCESS);


    /* Check the histogram and range */
    if (stats_flag->answer)
	check_stats(raster->answer);

    /* null file */
    if (null_flag->answer) {
	unsigned char *null_bits;
	int row, col;
	int fd;

	if (is_reclass)
	    G_fatal_error(_("[%s] is a reclass of another map. Exiting."),
			  raster->answer);

	/* Create a file of no-nulls */
	null_bits = Rast__allocate_null_bits(cellhd.cols);
	for (col = 0; col < Rast__null_bitstream_size(cellhd.cols); col++)
	    null_bits[col] = 0;

	/* Open null file for writing */
	Rast_set_window(&cellhd);
	fd = Rast__open_null_write(raster->answer);

	G_message(_("Writing new null file for [%s]... "), raster->answer);
	for (row = 0; row < cellhd.rows; row++) {
	    G_percent(row, cellhd.rows, 1);
	    Rast__write_null_bits(fd, null_bits);
	}
	G_percent(row, cellhd.rows, 1);

	/* Cleanup */
	Rast__close_null(fd);
	G_free(null_bits);
    }

    if (del_flag->answer) {
	char path[GPATH_MAX];

	if (is_reclass)
	    G_fatal_error(_("[%s] is a reclass of another map. Exiting."),
			  raster->answer);

	/* Write a file of no-nulls */
	G_message(_("Removing null file for [%s]...\n"), raster->answer);

	G_file_name_misc(path, "cell_misc", "null", raster->answer, G_mapset());
	unlink(path);
	G_file_name_misc(path, "cell_misc", "nullcmpr", raster->answer, G_mapset());
	unlink(path);

	G_done_msg(_("Done."));
    }

    return EXIT_SUCCESS;
}
