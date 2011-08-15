
/****************************************************************
 *
 * MODULE:       v.colors
 * 
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com>
 *               
 * PURPOSE:      Manage color tables for vector maps
 *               
 * COPYRIGHT:    (C) 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <string.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/raster.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>

#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct {
        struct Flag *r, *w, *l, *g, *a, *n, *e;
    } flag; 

    struct {
	struct Option *map, *field, *colr, *rast, *volume, *rules, *attrcol, *rgbcol;
    } opt;

    int layer, cmin, cmax;
    int have_stats;
    int overwrite, remove, is_from_stdin, stat, have_colors;
    const char *mapset, *cmapset;
    const char *style, *rules, *cmap, *attrcolumn, *rgbcolumn;
    char *name;
    
    struct Map_info Map;
    struct Colors colors, colors_tmp;
    /* struct Cell_stats statf; */
    
    G_gisinit(argv[0]);
    
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("color table"));
    module->description =
	_("Creates/modifies the color table associated with a vector map.");

    opt.map = G_define_standard_option(G_OPT_V_MAP);

    opt.field = G_define_standard_option(G_OPT_V_FIELD);

    opt.attrcol = G_define_standard_option(G_OPT_DB_COLUMN);
    opt.attrcol->label = _("Name of column containing numeric data");
    opt.attrcol->description = _("If not given categories are used");
    opt.colr = G_define_standard_option(G_OPT_M_COLR);
    
    opt.rgbcol = G_define_standard_option(G_OPT_DB_COLUMN);
    opt.rgbcol->key = "rgb_column";
    opt.rgbcol->label = _("Name of color column to populate RGB values");
    opt.rgbcol->description = _("If not given writes color table");

    opt.rast = G_define_standard_option(G_OPT_R_INPUT);
    opt.rast->key = "raster";
    opt.rast->required = NO;
    opt.rast->description =
        _("Raster map from which to copy color table");
    opt.rast->guisection = _("Define");

    opt.volume = G_define_standard_option(G_OPT_R3_INPUT);
    opt.volume->key = "volume";
    opt.volume->required = NO;
    opt.volume->description =
        _("3D raster map from which to copy color table");
    opt.volume->guisection = _("Define");

    opt.rules = G_define_standard_option(G_OPT_F_INPUT);
    opt.rules->key = "rules";
    opt.rules->required = NO;
    opt.rules->label = _("Path to rules file");
    opt.rules->description = _("\"-\" to read rules from stdin");
    opt.rules->guisection = _("Define");

    flag.r = G_define_flag();
    flag.r->key = 'r';
    flag.r->description = _("Remove existing color table");
    flag.r->guisection = _("Remove");

    flag.w = G_define_flag();
    flag.w->key = 'w';
    flag.w->description =
        _("Only write new color table if one doesn't already exist");

    flag.l = G_define_flag();
    flag.l->key = 'l';
    flag.l->description = _("List available rules then exit");
    flag.l->suppress_required = YES;
    flag.l->guisection = _("Print");

    flag.n = G_define_flag();
    flag.n->key = 'n';
    flag.n->description = _("Invert colors");
    flag.n->guisection = _("Define");

    flag.g = G_define_flag();
    flag.g->key = 'g';
    flag.g->description = _("Logarithmic scaling");
    flag.g->guisection = _("Define");

    flag.a = G_define_flag();
    flag.a->key = 'a';
    flag.a->description = _("Logarithmic-absolute scaling");
    flag.a->guisection = _("Define");

    flag.e = G_define_flag();
    flag.e->key = 'e';
    flag.e->description = _("Histogram equalization");
    flag.e->guisection = _("Define");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (flag.l->answer) {
	G_list_color_rules(stdout);
	return EXIT_SUCCESS;
    }

    overwrite = !flag.w->answer;
    remove = flag.r->answer;
    name = opt.map->answer;
    style = opt.colr->answer;
    rules = opt.rules->answer;
    attrcolumn = opt.attrcol->answer;
    rgbcolumn = opt.rgbcol->answer;
    have_stats = FALSE;
    
    if (!name)
        G_fatal_error(_("No vector map specified"));

    if (opt.rast->answer && opt.volume->answer)
        G_fatal_error(_("Options <%s> and <%s> are mutually exclusive"),
		      opt.rast->key, opt.volume->key);

    cmap = NULL;
    if (opt.rast->answer)
        cmap = opt.rast->answer;
    if (opt.volume->answer)
        cmap = opt.volume->answer;
    
    if (!cmap && !style && !rules && !remove)
        G_fatal_error(_("One of -%c or options <%s>, <%s> or <%s> "
			"must be specified"), flag.r->key, opt.colr->key,
			opt.rast->key, opt.rules->key);

    if (!!style + !!cmap + !!rules > 1)
        G_fatal_error(_("Options <%s>, <%s>, and <%s> are mutually "
			"exclusive"), opt.colr->key, opt.rules->key,
		      opt.rast->key);

    if (flag.g->answer && flag.a->answer)
        G_fatal_error(_("Flags -%c and -%c flags are mutually exclusive"),
		      flag.g->key, flag.a->key);

    is_from_stdin = rules && strcmp(rules, "-") == 0;
    if (is_from_stdin)
        rules = NULL;

    mapset = G_find_vector(name, "");
    if (!mapset)
	G_fatal_error(_("Vector map <%s> not found"), name);
    
    if (strcmp(mapset, G_mapset()) != 0)
      G_fatal_error(_("Module currently allows to modify only vector maps from the current mapset"));

    stat = -1;
    if (remove) {
	stat = Vect_remove_colors(name, mapset);
        if (stat < 0)
            G_fatal_error(_("Unable to remove color table of vector map <%s>"), name);
        if (stat == 0)
            G_warning(_("Color table of vector map <%s> not found"), name);
        return EXIT_SUCCESS;
    }

    G_suppress_warnings(TRUE);
    have_colors = Vect_read_colors(name, mapset, &colors);

    if (have_colors > 0 && !overwrite) {
        G_fatal_error(_("Color table exists. Exiting."));
    }

    G_suppress_warnings(FALSE);

    /* open map and get min/max values */
    Vect_open_old2(&Map, name, mapset, opt.field->answer);

    layer = Vect_get_field_number(&Map, opt.field->answer);
    if (layer < 1)
	G_fatal_error(_("Layer <%s> not found"), opt.field->answer);
    
    if (is_from_stdin) {
	/*
        if (!read_color_rules(stdin, &colors, min, max, fp))
            exit(EXIT_FAILURE);
	*/
    } else if (style) {	
	if (!G_find_color_rule(style))
	    G_fatal_error(_("Color table <%s> not found"), style);
	
	if (!attrcolumn) {
	    scan_cats(&Map, layer, style, &colors, &cmin, &cmax);
	}
	else {
	    scan_attr(&Map, layer, attrcolumn, style,
		      &colors, &cmin, &cmax);
	}
	
	if (strcmp(style, "random") == 0) {
	    Rast_make_random_colors(&colors, (CELL) cmin, (CELL) cmax);
	} else if (strcmp(style, "grey.eq") == 0) {
	    G_fatal_error(_("Color table <%s> not supported"), "grey.eq");
	    /*
	      if (!have_stats)
	      have_stats = get_stats(name, mapset, &statf);
	      Rast_make_histogram_eq_colors(&colors, &statf);
	    */
        } else if (strcmp(style, "grey.log") == 0) {
	    G_fatal_error(_("Color table <%s> not supported"), "grey.log");
	    /*
	      if (!have_stats)
	      have_stats = get_stats(name, mapset, &statf);
	      Rast_make_histogram_log_colors(&colors, &statf, (CELL) min,
	      (CELL) max);
	    */
	}
    }
    else if (rules) {
	if (!Rast_load_colors(&colors, rules, (CELL) cmin, (CELL) cmax))
	    G_fatal_error(_("Unable to load rules file <%s>"), rules);
    }
    else {
	/* use color from another map (cmap) */
	if (opt.rast->answer) {
            cmapset = G_find_raster2(cmap, "");
            if (!cmapset)
                G_fatal_error(_("Raster map <%s> not found"), cmap);

            if (Rast_read_colors(cmap, cmapset, &colors) < 0)
                G_fatal_error(_("Unable to read color table for raster map <%s>"), cmap);
        } else {
            cmapset = G_find_raster3d(cmap, "");
            if (!cmapset)
                G_fatal_error(_("3D raster map <%s> not found"), cmap);

            if (Rast3d_read_colors(cmap, cmapset, &colors) < 0)
                G_fatal_error(_("Unable to read color table for 3D raster map <%s>"), cmap);
        }
    }

    if (flag.n->answer)
        Rast_invert_colors(&colors);

    if (flag.e->answer) {
	G_fatal_error(_("Flag -%c needs to be implemeneted"));
	/*
	if (!have_stats)
	    have_stats = get_stats(name, mapset, &statf);
	Rast_histogram_eq_colors(&colors_tmp, &colors, &statf);
	colors = colors_tmp;
	*/
    }

    if (flag.g->answer) {
        Rast_log_colors(&colors_tmp, &colors, 100);
        colors = colors_tmp;
    }

    if (flag.a->answer) {
        Rast_abs_log_colors(&colors_tmp, &colors, 100);
        colors = colors_tmp;
    }

    if (rgbcolumn)
	write_rgb_values(&Map, layer, rgbcolumn, &colors);
    else
	Vect_write_colors(name, mapset, &colors);

    Vect_close(&Map);
    
    G_message(_("Color table for vector map <%s> set to '%s'"), 
	      G_fully_qualified_name(name, mapset), 
              is_from_stdin ? "rules" : style ? style : rules ? rules :
              cmap);
    
    exit(EXIT_SUCCESS);
}
