
/****************************************************************
 *
 * MODULE:       v.colors
 * 
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com>
 *               
 * PURPOSE:      Manage color tables for vector maps
 *               
 * COPYRIGHT:    (C) 2011-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <stdlib.h>
#include <string.h>

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
	struct Flag *r, *w, *l, *d, *g, *a, *n, *c;
    } flag; 

    struct {
	struct Option *map, *field, *colr, *rast, *volume, *rules,
          *attrcol, *rgbcol, *range, *use;
    } opt;

    int layer;
    int overwrite, remove, is_from_stdin, stat, have_colors, convert, use;
    const char *mapset, *cmapset;
    const char *style, *rules, *cmap, *attrcolumn, *rgbcolumn;
    char *name;
    
    struct Map_info Map;
    struct FPRange range;
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

    opt.use = G_define_option();
    opt.use->key = "use";
    opt.use->type = TYPE_STRING;
    opt.use->required = YES;
    opt.use->multiple = NO;
    opt.use->options = "attr,cat,z";
    opt.use->description = _("Source values");
    G_asprintf((char **) &(opt.use->descriptions),
	       "attr;%s;cat;%s;z;%s",
	       _("read values from attribute table (requires <column> option)"),
	       _("use category values"),
	       _("use z coordinate (3D points or centroids only)"));
    opt.use->answer = "cat";
    
    opt.attrcol = G_define_standard_option(G_OPT_DB_COLUMN);
    opt.attrcol->label = _("Name of column containing numeric data");
    opt.attrcol->description = _("Required for use=attr");
    opt.attrcol->guisection = _("Define");

    opt.range = G_define_option();
    opt.range->key = "range";
    opt.range->type = TYPE_DOUBLE;
    opt.range->required = NO;
    opt.range->label = _("Manually set range (refers to 'column' option)");
    opt.range->description = _("Ignored when 'rules' given");
    opt.range->key_desc = "min,max";

    opt.colr = G_define_standard_option(G_OPT_M_COLR);
    opt.colr->guisection = _("Define");

    opt.rast = G_define_standard_option(G_OPT_R_INPUT);
    opt.rast->key = "raster";
    opt.rast->required = NO;
    opt.rast->description =
        _("Raster map from which to copy color table");
    opt.rast->guisection = _("Define");

    opt.volume = G_define_standard_option(G_OPT_R3_INPUT);
    opt.volume->key = "raster_3d";
    opt.volume->required = NO;
    opt.volume->description =
        _("3D raster map from which to copy color table");
    opt.volume->guisection = _("Define");

    opt.rules = G_define_standard_option(G_OPT_F_INPUT);
    opt.rules->key = "rules";
    opt.rules->required = NO;
    opt.rules->description = _("Path to rules file");
    opt.rules->guisection = _("Define");

    opt.rgbcol = G_define_standard_option(G_OPT_DB_COLUMN);
    opt.rgbcol->key = "rgb_column";
    opt.rgbcol->label = _("Name of color column to populate RGB values");
    opt.rgbcol->description = _("If not given writes color table");
    
    flag.r = G_define_flag();
    flag.r->key = 'r';
    flag.r->description = _("Remove existing color table");
    flag.r->guisection = _("Remove");

    flag.w = G_define_flag();
    flag.w->key = 'w';
    flag.w->description =
        _("Only write new color table if it does not already exist");

    flag.l = G_define_flag();
    flag.l->key = 'l';
    flag.l->description = _("List available rules then exit");
    flag.l->suppress_required = YES;
    flag.l->guisection = _("Print");

    flag.d = G_define_flag();
    flag.d->key = 'd';
    flag.d->label = _("List available rules with description then exit");
    flag.d->description = _("If a color rule is given, only this rule is listed");
    flag.d->suppress_required = YES;
    flag.d->guisection = _("Print");

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

    flag.c = G_define_flag();
    flag.c->key = 'c';
    flag.c->label = _("Convert color rules from RGB values to color table");
    flag.c->description = _("Option 'rgb_column' with valid RGB values required");
	
    /* TODO ?
    flag.e = G_define_flag();
    flag.e->key = 'e';
    flag.e->description = _("Histogram equalization");
    flag.e->guisection = _("Define");
    */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (flag.l->answer) {
	G_list_color_rules(stdout);
	return EXIT_SUCCESS;
    }

    if (flag.d->answer) {
	G_list_color_rules_description_type(stdout, opt.colr->answer);

        return EXIT_SUCCESS;
    }

    overwrite = !flag.w->answer;
    remove = flag.r->answer;
    name = opt.map->answer;
    style = opt.colr->answer;
    rules = opt.rules->answer;
    attrcolumn = opt.attrcol->answer;
    rgbcolumn = opt.rgbcol->answer;
    convert = flag.c->answer;
    use = USE_CAT;
    if (opt.use->answer) {
        switch (opt.use->answer[0]) {
        case 'a':
            use = USE_ATTR;
            break;
        case 'c':
            use = USE_CAT;
            break;
        case 'z':
            use = USE_Z;
            break;
        default:
            break;
        }
    }
    G_debug(1, "use=%d", use);
    
    if (!name)
        G_fatal_error(_("No vector map specified"));

    if (use == USE_ATTR && !attrcolumn)
        G_fatal_error(_("Option <%s> required"), opt.attrcol->key); 
    if (use != USE_ATTR && attrcolumn) {
        G_important_message(_("Option <%s> given, assuming <use=attr>..."), opt.attrcol->key);
        use = USE_ATTR;
    }

    if (opt.rast->answer && opt.volume->answer)
        G_fatal_error(_("%s= and %s= are mutually exclusive"),
		      opt.rast->key, opt.volume->key);

    cmap = NULL;
    if (opt.rast->answer)
        cmap = opt.rast->answer;
    if (opt.volume->answer)
        cmap = opt.volume->answer;
    
    if (!cmap && !style && !rules && !remove && !convert)
        G_fatal_error(_("One of -%c, -%c or %s=, %s= or %s= "
			"must be specified"), flag.r->key, flag.c->key, 
		      opt.colr->key, opt.rast->key, opt.rules->key);
    
    if (!!style + !!cmap + !!rules > 1)
        G_fatal_error(_("%s=, %s= and %s= are mutually exclusive"),
			opt.colr->key, opt.rules->key, opt.rast->key);

    if (flag.g->answer && flag.a->answer)
        G_fatal_error(_("-%c and -%c are mutually exclusive"),
		      flag.g->key, flag.a->key);

    if (flag.c->answer && !rgbcolumn) 
	G_fatal_error(_("%s= required for -%c"),
		      opt.rgbcol->key, flag.c->key);

    is_from_stdin = rules && strcmp(rules, "-") == 0;
    if (is_from_stdin)
        G_fatal_error(_("Reading rules from standard input is not implemented yet, please provide path to rules file instead."));

    mapset = G_find_vector(name, "");
    if (!mapset)
	G_fatal_error(_("Vector map <%s> not found"), name);
    
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
    have_colors = Vect_read_colors(name, mapset, NULL);

    if (have_colors > 0 && !overwrite) {
        G_fatal_error(_("Color table exists. Exiting."));
    }

    G_suppress_warnings(FALSE);

    /* open map and get min/max values */
    Vect_set_open_level(1); /* no topology required */
    if (Vect_open_old2(&Map, name, mapset, opt.field->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), name);

    Vect_set_error_handler_io(&Map, NULL);
    if (use == USE_Z && !Vect_is_3d(&Map))
        G_fatal_error(_("Vector map <%s> is not 3D"), Vect_get_full_name(&Map));
    
    layer = Vect_get_field_number(&Map, opt.field->answer);
    if (layer < 1)
	G_fatal_error(_("Layer <%s> not found"), opt.field->answer);
    
    if (opt.range->answer) {
	range.min = atof(opt.range->answers[0]);
	range.max = atof(opt.range->answers[1]);
	if (range.min > range.max)
	    G_fatal_error(_("Option <%s>: min must be greater or equal to max"),
			  opt.range->key);
    }

    Rast_init_colors(&colors);
    if (is_from_stdin) {
        G_fatal_error(_("Reading color rules from standard input is currently not supported"));
	/*
        if (!read_color_rules(stdin, &colors, min, max, fp))
            exit(EXIT_FAILURE);
	*/
    } else if (style || rules) {	
	if (style && !G_find_color_rule(style))
	    G_fatal_error(_("Color table <%s> not found"), style);
	
	if (use == USE_CAT) {
	    scan_cats(&Map, layer, style, rules,
		      opt.range->answer ? &range : NULL,
		      &colors);
        }
        else if (use == USE_Z) {
	    scan_z(&Map, layer, style, rules,
		      opt.range->answer ? &range : NULL,
		      &colors);
        }
        else {
	    scan_attr(&Map, layer, attrcolumn, style, rules,
		      opt.range->answer ? &range : NULL,
		      &colors);
	}
    }
    else {
	/* use color from another map (cmap) */
	if (opt.rast->answer) {
            cmapset = G_find_raster2(cmap, "");
            if (!cmapset)
                G_fatal_error(_("Raster map <%s> not found"), cmap);

            if (Rast_read_colors(cmap, cmapset, &colors) < 0)
                G_fatal_error(_("Unable to read color table for raster map <%s>"), cmap);
        } else if (opt.volume->answer) {
            cmapset = G_find_raster3d(cmap, "");
            if (!cmapset)
                G_fatal_error(_("3D raster map <%s> not found"), cmap);

            if (Rast3d_read_colors(cmap, cmapset, &colors) < 0)
                G_fatal_error(_("Unable to read color table for 3D raster map <%s>"), cmap);
        }
    }

    if (flag.n->answer)
        Rast_invert_colors(&colors);

    /* TODO ?
    if (flag.e->answer) {
    if (!have_stats)
    have_stats = get_stats(name, mapset, &statf);
    Rast_histogram_eq_colors(&colors_tmp, &colors, &statf);
    colors = colors_tmp;
    }
    */
    if (flag.g->answer) {
        Rast_log_colors(&colors_tmp, &colors, 100);
        colors = colors_tmp;
    }

    if (flag.a->answer) {
        Rast_abs_log_colors(&colors_tmp, &colors, 100);
        colors = colors_tmp;
    }

    G_important_message(_("Writing color rules..."));
    
    if (style || rules || opt.rast->answer || opt.volume->answer) {
	if (rgbcolumn)
	    write_rgb_values(&Map, layer, rgbcolumn, &colors);
	else
	    Vect_write_colors(name, mapset, &colors);
    }
    
    if (convert) {
	/* convert RGB values to color tables */
	rgb2colr(&Map, layer, rgbcolumn, &colors);
	Vect_write_colors(name, mapset, &colors);
    }
    Vect_close(&Map);
    
    G_message(_("Color table for vector map <%s> set to '%s'"), 
	      G_fully_qualified_name(name, mapset), 
              is_from_stdin || convert ? "rules" : style ? style : rules ? rules :
              cmap);
    
    exit(EXIT_SUCCESS);
}
