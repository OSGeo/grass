
/**********************************************************************
 *
 *   G_make_color (name, mapset, colors)
 *       char *name               name of map
 *       char *mapset             mapset containing name
 *       struct Colors *colors    struct to hold colors
 *
 *   Interactively prompts user for deciding which type of color
 *   lookup table is desired.
 *       Red, green, and blue color ramps
 *       Gray scale
 *       Rainbow colors
 *       Random colors
 *       Color wave
 *       Aspect colors
 *       Red through yellow to green
 *
 *   Returns -1 user canceled the request
 *            1 color table is ok
 **********************************************************************/

#include <grass/gis.h>
#include <grass/glocale.h>

int G_ask_colors(const char *name, const char *mapset, struct Colors *pcolr)
{
    char buff[128];
    int answ;
    struct FPRange range;
    DCELL min, max;

    G_init_colors(pcolr);

    /* determine range cell values */
    if (G_read_fp_range(name, mapset, &range) < 0)
	return -1;
    G_get_fp_range_min_max(&range, &min, &max);
    if (G_is_d_null_value(&min) || G_is_d_null_value(&max)) {
	sprintf(buff, _(" The raster map %s@%s is empty"), name, mapset);
	G_warning(buff);
	return -1;
    }

    /* Prompting */
  ASK:
    G_clear_screen();
    fprintf(stderr,
	    _("\n\nColor table needed for file [%s] in mapset [%s].\n"), name,
	    mapset);

    fprintf(stderr, _("\nPlease identify the type desired:\n"));
    fprintf(stderr, _("    1:  Random colors\n"));
    fprintf(stderr, _("    2:  Red, green, and blue color ramps\n"));
    fprintf(stderr, _("    3:  Color wave\n"));
    fprintf(stderr, _("    4:  Gray scale\n"));
    fprintf(stderr, _("    5:  Aspect\n"));
    fprintf(stderr, _("    6:  Rainbow colors\n"));
    fprintf(stderr, _("    7:  Red through yellow to green\n"));
    fprintf(stderr, _("    8:  Green through yellow to red\n"));
    fprintf(stderr, _("RETURN  quit\n"));
    fprintf(stderr, "\n> ");

    for (;;) {
	if (!G_gets(buff))
	    goto ASK;
	G_strip(buff);
	if (*buff == 0)
	    return -1;
	if (sscanf(buff, "%d", &answ) != 1)
	    answ = -1;

	switch (answ) {
	case 1:
	    return G_make_random_colors(pcolr, (CELL) min, (CELL) max);
	case 2:
	    return G_make_ramp_fp_colors(pcolr, min, max);
	case 3:
	    return G_make_wave_fp_colors(pcolr, min, max);
	case 4:
	    return G_make_grey_scale_fp_colors(pcolr, min, max);
	case 5:
	    return G_make_aspect_fp_colors(pcolr, min, max);
	case 6:
	    return G_make_rainbow_fp_colors(pcolr, min, max);
	case 7:
	    return G_make_ryg_fp_colors(pcolr, min, max);
	case 8:
	    return G_make_gyr_fp_colors(pcolr, min, max);
	default:
	    fprintf(stderr, _("\n%s invalid; Try again > "), buff);
	    break;
	}
    }
}
