#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

void make_colors(struct Colors *colors, const char *style, DCELL min, DCELL max, int is_fp)
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

void load_colors(struct Colors *colors, const char *rules, DCELL min, DCELL max, int is_fp)
{
    int ret;
    
    if (is_fp)
	ret = Rast_load_fp_colors(colors, rules, (DCELL) min, (DCELL) max);
    else
	ret = Rast_load_colors(colors, rules, (CELL) min, (CELL) max);

    if (ret == 0)
	G_fatal_error(_("Unable to load rules file <%s>"), rules);
}
