/*!
  \file lib/raster/color_read.c
  
  \brief Raster Library - Read color table of raster map
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author USACERL and many others
*/

#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static int read_new_colors(FILE *, struct Colors *);
static int read_old_colors(FILE *, struct Colors *);


/*!
  \brief Read color table of raster map
  
  The color table for the raster map <i>name</i> in the specified
  <i>mapset</i> is read into the <i>colors</i> structure. If the data
  layer has no color table, a default color table is generated and 0
  is returned. If there is an error reading the color table, a
  diagnostic message is printed and -1 is returned. If the color
  table is read ok, 1 is returned.
  
  This routine reads the rules from the color file. If the input
  raster map is is a floating-point map it calls
  Rast_mark_colors_as_fp().
  
  Note: If a secondary color file for map name <i>name</i> exists in
  the current project, that color file is read.  This allows the
  user to define their own color lookup tables for cell maps found
  in other mapsets.
  
  Warning message is printed if the color file is
  missing or invalid.
  
  \param name map name
  \param mapset mapset name
  \param[out] colors pointer to Colors structure
  
  \return -1 on error
  \return 0 if missing, but default colors generated
  \return 1 on success
*/
int Rast_read_colors(const char *name, const char *mapset,
		     struct Colors *colors)
{
    int fp;
    char buf[GNAME_MAX];
    char *err;
    char xname[GNAME_MAX];
    struct Range range;
    struct FPRange drange;
    CELL min, max;
    DCELL dmin, dmax;

    fp = Rast_map_is_fp(name, mapset);
    Rast_init_colors(colors);

    strcpy(xname, name);
    mapset = G_find_raster(xname, mapset);
    name = xname;

    if (fp)
	Rast_mark_colors_as_fp(colors);

    /* first look for secondary color table in current mapset */
    sprintf(buf, "colr2/%s", mapset);
    if (Rast__read_colors(buf, name, G_mapset(), colors) >= 0)
	return 1;

    /* now look for the regular color table */
    switch (Rast__read_colors("colr", name, mapset, colors)) {
    case -2:
	if (!fp) {
	    if (Rast_read_range(name, mapset, &range) >= 0) {
		Rast_get_range_min_max(&range, &min, &max);
		if (!Rast_is_c_null_value(&min) &&
		    !Rast_is_c_null_value(&max))
		    Rast_make_colors(colors, DEFAULT_COLOR_TABLE, min, max);
		return 0;
	    }
	}
	else {
	    if (Rast_read_fp_range(name, mapset, &drange) >= 0) {
		Rast_get_fp_range_min_max(&drange, &dmin, &dmax);
		if (!Rast_is_d_null_value(&dmin) &&
		    !Rast_is_d_null_value(&dmax))
		    Rast_make_fp_colors(colors, DEFAULT_COLOR_TABLE, dmin, dmax);
		return 0;
	    }
	}
	err = _("missing");
	break;
    case -1:
	err = _("invalid");
	break;
    default:
	return 1;
    }

    G_warning(_("Color support for <%s@%s> %s"), name, mapset, err);
    return -1;
}

int Rast__read_colors(const char *element, const char *name,
		      const char *mapset, struct Colors *colors)
{
    FILE *fd;
    int stat;
    char buf[1024];

    if (!(fd = G_fopen_old(element, name, mapset)))
	return -2;

    /*
     * first line in 4.0 color files is %
     * otherwise it is pre 4.0
     */
    if (fgets(buf, sizeof buf, fd) == NULL) {
	fclose(fd);
	return -1;
    }
    
    stat = 1;
    if (colors) {
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
 *    nv:r:g:b          color to use for NULL values
 *    *:r:g:b           color to use for undefined (beyond color rules)
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
		Rast_add_modular_c_color_rule((CELL *) & cat1, r1, g1, b1,
					      (CELL *) & cat2, r2, g2, b2,
					      colors);
	}
	else {
	    if (fp_rule)
		Rast_add_d_color_rule((DCELL *) & val1, r1, g1, b1,
				      (DCELL *) & val2, r2, g2, b2, colors);
	    else
		Rast_add_c_color_rule((CELL *) & cat1, r1, g1, b1,
				      (CELL *) & cat2, r2, g2, b2, colors);
	}
	G_debug(3, "adding rule %ld=%.2lf %d %d %d  %ld=%.2lf %d %d %d",
		cat1, val1, r1, g1, b1, cat2, val2, r2, g2, b2);
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

/*!
 * \brief Mark colors as floating-point.
 *
 * Sets a flag in the <i>colors</i> structure that indicates that
 * these colors should only be looked up using floating-point raster
 * data (not integer data). In particular if this flag is set, the
 * routine Rast_get_c_colors_min_max() should return min=-255$^3$ and
 * max=255$^3$.
 *
 * \param colors pointer to Colors structure
 */
void Rast_mark_colors_as_fp(struct Colors *colors)
{
    colors->is_float = 1;
}
