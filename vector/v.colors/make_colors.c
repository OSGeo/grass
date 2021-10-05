#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

void make_colors(struct Colors *colors, const char *style, DCELL min, DCELL max,
		 int is_fp)
{

    G_debug(3, "make_colors(): range=%f,%f is_fp=%d", min, max, is_fp);

    if (strcmp(style, "random") == 0) {
	if (is_fp)
	    G_fatal_error(_("Color table '%s' is not supported for "
			    "floating point attributes"), style);
	Rast_make_random_colors(colors, (CELL) min, (CELL) max);
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
    else {
	if (is_fp)
	    Rast_make_fp_colors(colors, style, min, max);
	else
	    Rast_make_colors(colors, style, (CELL) min, (CELL) max);
    }
}

void load_colors(struct Colors *colors, const char *rules, DCELL min, DCELL max,
		 int is_fp)
{
    int ret;

    if (rules[0] == '-' && rules[1] == 0)
	ret = Rast_read_color_rules(colors, min, max, Rast_read_color_rule,
				    stdin);
    else if (is_fp)
	ret = Rast_load_fp_colors(colors, rules, (DCELL) min, (DCELL) max);
    else
	ret = Rast_load_colors(colors, rules, (CELL) min, (CELL) max);

    if (ret == 0)
	G_fatal_error(_("Unable to load rules file <%s>"), rules);
}

void color_rules_to_cats(dbCatValArray *cvarr, int is_fp,
			 struct Colors *vcolors, struct Colors *colors,
			 int invert, DCELL min, DCELL max)
{
    int i, cat;
    dbCatVal *cv;
    int red, grn, blu;

    /* color table for categories */
    G_message(_("Converting color rules into categories..."));
    for (i = 0; i < cvarr->n_values; i++) {
	G_percent(i, cvarr->n_values, 2);
	cv = &(cvarr->value[i]);
	cat = cv->cat;
	if (is_fp) {
	    DCELL v = invert ? min + max - cv->val.d : cv->val.d;
	    if (Rast_get_d_color((const DCELL *) &v, &red, &grn, &blu,
				 vcolors) == 0) {
		/* G_warning(_("No color rule defined for value %f"), v); */
		G_debug(3, "scan_attr(): cat=%d, val=%f -> no color rule",
			cat, v);
		continue;
	    }
	}
	else {
	    CELL v = invert ? (CELL) min + (CELL) max - cv->val.i : cv->val.i;
	    if (Rast_get_c_color((const CELL *) &v, &red, &grn, &blu,
				 vcolors) == 0) {
		/* G_warning(_("No color rule defined for value %d"), v); */
		G_debug(3, "scan_attr(): cat=%d, val=%d -> no color rule",
			cat, v);
		continue;
	    }
	}
	G_debug(3, "scan_attr(): cat=%d, val=%f, r=%d, g=%d, b=%d",
		cat, is_fp ? cv->val.d : cv->val.i, red, grn, blu);
	Rast_add_c_color_rule((const CELL*) &cat, red, grn, blu,
			      (const CELL*) &cat, red, grn, blu, colors);
    }
    G_percent(2, 2, 2);
}
