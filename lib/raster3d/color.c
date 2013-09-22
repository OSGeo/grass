
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "raster3d_intern.h"

static int read_colors(const char *, const char *, struct Colors *);
static int read_new_colors(FILE *, struct Colors *);
static int read_old_colors(FILE *, struct Colors *);

/*---------------------------------------------------------------------------*/

/*!
   \brief Removes the primary and/or secondary color file.

   \todo Is <em>primary and/or secondary color file</em> still valid for 7?

   \see G_remove_colr
*/
int Rast3d_remove_color(const char *name)
 /* adapted from G_remove_colr */
{
    return G_remove_misc(RASTER3D_DIRECTORY, RASTER3D_COLOR_ELEMENT, name);
}

/*---------------------------------------------------------------------------*/

/*!
   \brief Reads color file for map \p name in \p mapset into the Colors structure.

   \param name 3D raster map name
   \param mapset mapset name
   \param colors colors to be associated with a map

   \see Rast3d_write_colors, Rast_read_colors
*/
int Rast3d_read_colors(const char *name, const char *mapset, struct Colors *colors)
 /* adapted from Rast_read_colors */
{
    const char *err;
    struct FPRange drange;
    DCELL dmin, dmax;

    Rast_init_colors(colors);

    Rast_mark_colors_as_fp(colors);

    switch (read_colors(name, mapset, colors)) {
    case -2:
	if (Rast3d_read_range(name, mapset, &drange) >= 0) {
	    Rast_get_fp_range_min_max(&drange, &dmin, &dmax);
	    if (!Rast_is_d_null_value(&dmin) && !Rast_is_d_null_value(&dmax))
		Rast_make_rainbow_fp_colors(colors, dmin, dmax);
	    return 0;
	}
	err = "missing";
	break;
    case -1:
	err = "invalid";
	break;
    default:
	return 1;
    }

    G_warning("color support for [%s] in mapset [%s] %s", name, mapset, err);
    return -1;
}

static int read_colors(const char *name, const char *mapset,
		       struct Colors *colors)
{
    FILE *fd;
    int stat;
    char buf[1024];

    fd = G_fopen_old_misc(RASTER3D_DIRECTORY, RASTER3D_COLOR_ELEMENT, name, mapset);
    if (!fd)
	return -2;

    /*
     * first line in 4.0 color files is %
     * otherwise it is pre 4.0
     */
    if (fgets(buf, sizeof buf, fd) == NULL) {
	fclose(fd);
	return -1;
    }
    G_fseek(fd, 0L, 0);

    G_strip(buf);
    if (*buf == '%') {		/* 4.0 format */
	stat = read_new_colors(fd, colors);
	colors->version = 0;	/* 4.0 format */
    }
    else {
	stat = read_old_colors(fd, colors);
	colors->version = -1;	/* pre 4.0 format */
    }
    fclose(fd);
    return stat;
}

/* parse input lines with the following formats
 *   val1:r:g:b val2:r:g:b
 *   val:r:g:b          (implies cat1==cat2)
 *
 * r:g:b can be just a single grey level
 *   cat1:x cat2:y
 *   cat:x
 *
 * optional lines are
 *    invert            invert color table
 *    shift:n           where n is the amount to shift the color table
 */
static int read_new_colors(FILE * fd, struct Colors *colors)
{
    double val1, val2;
    long cat1, cat2;
    int r1, g1, b1;
    int r2, g2, b2;
    char buf[1024];
    char word1[256], word2[256];
    int n, fp_rule;
    int null, undef;
    int modular;
    DCELL shift;

    if (fgets(buf, sizeof buf, fd) == NULL)
	return -1;
    G_strip(buf);

    if (sscanf(buf + 1, "%lf %lf", &val1, &val2) == 2)
	Rast_set_d_color_range((DCELL) val1, (DCELL) val2, colors);

    modular = 0;
    while (fgets(buf, sizeof buf, fd)) {
	null = undef = fp_rule = 0;
	*word1 = *word2 = 0;
	n = sscanf(buf, "%s %s", word1, word2);
	if (n < 1)
	    continue;

	if (sscanf(word1, "shift:%lf", &shift) == 1
	    || (strcmp(word1, "shift:") == 0 &&
		sscanf(word2, "%lf", &shift) == 1)) {
	    Rast_shift_d_colors(shift, colors);
	    continue;
	}
	if (strcmp(word1, "invert") == 0) {
	    Rast_invert_colors(colors);
	    continue;
	}
	if (strcmp(word1, "%%") == 0) {
	    modular = !modular;
	    continue;
	}

	switch (sscanf(word1, "nv:%d:%d:%d", &r1, &g1, &b1)) {
	case 1:
	    null = 1;
	    b1 = g1 = r1;
	    break;
	case 3:
	    null = 1;
	    break;
	}
	if (!null)
	    switch (sscanf(word1, "*:%d:%d:%d", &r1, &g1, &b1)) {
	    case 1:
		undef = 1;
		b1 = g1 = r1;
		break;
	    case 3:
		undef = 1;
		break;
	    }
	if (!null && !undef)
	    switch (sscanf(word1, "%ld:%d:%d:%d", &cat1, &r1, &g1, &b1)) {
	    case 2:
		b1 = g1 = r1;
		break;
	    case 4:
		break;
	    default:
		if (sscanf(word1, "%lf:%d:%d:%d", &val1, &r1, &g1, &b1) == 4)
		    fp_rule = 1;
		else if (sscanf(word1, "%lf:%d", &val1, &r1) == 2) {
		    fp_rule = 1;
		    b1 = g1 = r1;
		}
		else
		    continue;	/* other lines are ignored */
	    }
	if (n == 2) {
	    switch (sscanf(word2, "%ld:%d:%d:%d", &cat2, &r2, &g2, &b2)) {
	    case 2:
		b2 = g2 = r2;
		if (fp_rule)
		    val2 = (DCELL) cat2;
		break;
	    case 4:
		if (fp_rule)
		    val2 = (DCELL) cat2;
		break;
	    default:
		if (sscanf(word2, "%lf:%d:%d:%d", &val2, &r2, &g2, &b2) == 4) {
		    if (!fp_rule)
			val1 = (DCELL) cat1;
		    fp_rule = 1;
		}
		else if (sscanf(word2, "%lf:%d", &val2, &r2) == 2) {
		    if (!fp_rule)
			val1 = (DCELL) cat1;
		    fp_rule = 1;
		    b2 = g2 = r2;
		}
		else
		    continue;	/* other lines are ignored */
	    }
	}
	else {
	    if (!fp_rule)
		cat2 = cat1;
	    else
		val2 = val1;
	    r2 = r1;
	    g2 = g1;
	    b2 = b1;
	}
	if (null)
	    Rast_set_null_value_color(r1, g1, b1, colors);
	else if (undef)
	    Rast_set_default_color(r1, g1, b1, colors);

	else if (modular) {
	    if (fp_rule)
		Rast_add_modular_d_color_rule((DCELL *) & val1, r1, g1,
						  b1, (DCELL *) & val2, r2,
						  g2, b2, colors);
	    else
		Rast_add_modular_c_color_rule((CELL *) &cat1, r1, g1, b1,
					      (CELL *) &cat2, r2, g2, b2, colors);
	}
	else {
	    if (fp_rule)
		Rast_add_d_color_rule((DCELL *) & val1, r1, g1, b1,
				      (DCELL *) & val2, r2, g2, b2,
				      colors);
	    else
		Rast_add_c_color_rule((CELL *) &cat1, r1, g1, b1,
				      (CELL *) &cat2, r2, g2, b2, colors);
	}
	/*
	   fprintf (stderr, "adding rule %d=%.2lf %d %d %d  %d=%.2lf %d %d %d\n", cat1,val1,  r1, g1, b1, cat2, val2, r2, g2, b2);
	 */
    }
    return 1;
}

static int read_old_colors(FILE * fd, struct Colors *colors)
{
    char buf[256];
    long n;
    long min;
    float red_f, grn_f, blu_f;
    int red, grn, blu;
    int old;
    int zero;

    Rast_init_colors(colors);
    /*
     * first line in pre 3.0 color files is number of colors - ignore
     * otherwise it is #min first color, and the next line is for color 0
     */
    if (fgets(buf, sizeof buf, fd) == NULL)
	return -1;

    G_strip(buf);
    if (*buf == '#') {		/* 3.0 format */
	old = 0;
	if (sscanf(buf + 1, "%ld", &min) != 1)	/* first color */
	    return -1;
	zero = 1;
    }
    else {
	old = 1;
	min = 0;
	zero = 0;
    }

    colors->cmin = min;
    n = min;
    while (fgets(buf, sizeof buf, fd)) {
	if (old) {
	    if (sscanf(buf, "%f %f %f", &red_f, &grn_f, &blu_f) != 3)
		return -1;

	    red = 256 * red_f;
	    grn = 256 * grn_f;
	    blu = 256 * blu_f;
	}
	else {
	    switch (sscanf(buf, "%d %d %d", &red, &grn, &blu)) {
	    case 1:
		blu = grn = red;
		break;
	    case 2:
		blu = grn;
		break;
	    case 3:
		break;
	    default:
		return -1;
	    }
	}
	if (zero) {
	    Rast__insert_color_into_lookup((CELL) 0, red, grn, blu,
					&colors->fixed);
	    zero = 0;
	}
	else
	    Rast__insert_color_into_lookup((CELL) n++, red, grn, blu,
					&colors->fixed);
    }
    colors->cmax = n - 1;

    return 0;
}

/*---------------------------------------------------------------------------*/

/*!
   \brief Writes the \p colors for map \p name in \p mapset into a color file.

   \param name 3D raster map name
   \param mapset mapset name
   \param colors colors to be associated with a map

   \see Rast3d_read_colors, Rast3d_remove_color, Rast_write_colors
*/
int Rast3d_write_colors(const char *name, const char *mapset, struct Colors *colors)
 /* adapted from Rast_write_colors */
{
    FILE *fd;

    if (strcmp(mapset, G_mapset()) != 0) {
	G_warning(_("mapset <%s> is not the current mapset"), mapset);
	return -1;
    }

    fd = G_fopen_new_misc(RASTER3D_DIRECTORY, RASTER3D_COLOR_ELEMENT, name);
    if (!fd)
	return -1;

    Rast__write_colors(fd, colors);
    fclose(fd);

    return 1;
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
