/*!
  \file lib/raster/color_rules.c
  
  \brief Raster Library - Read and parse color rules file
  
  (C) 2007-2016 by the GRASS Development Team
  
  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author Glynn Clements
*/

#include <stdio.h>

#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/glocale.h>

struct rule
{
    int set;
    int r, g, b;
    DCELL val;
};

enum rule_error
{
    CR_OK = 0,
    CR_ERROR_RULE_SYNTAX,
    CR_ERROR_COLOR_SYNTAX,
    CR_ERROR_PERCENT,
    CR_ERROR_VALUE,
};

/*!
  \brief Read color rule

  The val output parameter is always an absolute value and is derived
  from the rule and the min and max values in case the rule is in percents.

  Always only one of the norm, nval, and dflt output parameters is set
  to non-zero value, the others are set to zero.

  The return code can be translated to an error message using
  the Rast_parse_color_rule_error() function.

  \param min, max min & max values (used only when color rules are in percentage)
  \param buf string with the color rule
  \param[out] val value which the color is assigned to
  \param[out] r,g,b color values
  \param[out] norm set to non-zero value if the value and color are set
  \param[out] nval set to non-zero value if rule is for null value
  \param[out] dflt set to non-zero value if rule specifies the default color

  \returns enum rule_error values (non-zero on failure)
*/
int Rast_parse_color_rule(DCELL min, DCELL max, const char *buf,
			  DCELL * val, int *r, int *g, int *b,
			  int *norm, int *nval, int *dflt)
{
    char value[80], color[80];
    double x;
    char c;

    *norm = *nval = *dflt = 0;

    if (sscanf(buf, "%s %[^\n]", value, color) != 2)
	return CR_ERROR_RULE_SYNTAX;

    /* we don't allow 'none' color here (ret == 2) */
    /* G_str_to_color chops and has only 1 error state */
    if (G_str_to_color(color, r, g, b) != 1)
	    return CR_ERROR_COLOR_SYNTAX;

    G_chop(value);

    if (G_strcasecmp(value, "default") == 0) {
	*dflt = 1;
	return CR_OK;
    }

    if (G_strcasecmp(value, "nv") == 0) {
	*nval = 1;
	return CR_OK;
    }

    if (sscanf(value, "%lf%c", &x, &c) == 2 && c == '%') {
	if (x < 0 || x > 100)
	    return CR_ERROR_PERCENT;

	*val = min + (max - min) * (x / 100);
	*norm = 1;
	return CR_OK;
    }

    if (sscanf(value, "%lf", val) == 1) {
	*norm = 1;
	return CR_OK;
    }

    return CR_ERROR_VALUE;
}

/*!
  \brief Parse color rule

  \param code
  
  \return pointer to buffer with error message
*/
const char *Rast_parse_color_rule_error(int code)
{
    switch (code) {
    case CR_OK:
	return "";
    case CR_ERROR_RULE_SYNTAX:
	return _("syntax error in the color rule");
    case CR_ERROR_COLOR_SYNTAX:
	return _("syntax error in the color format");
/* we no longer distinguish between these two errors
    case CR_ERROR_RGB:
	return _("R/G/B not in range 0-255");
    case CR_ERROR_COLOR:
	return _("invalid color name");
*/
    case CR_ERROR_PERCENT:
	return _("percentage not in range 0-100");
    case CR_ERROR_VALUE:
	return _("invalid value");
    default:
	return _("unknown error");
    }
}

/*!
  \brief Read color rule

  \param closure
  \param min, max min & max values (used only when color rules are in percentage)
  \param val value
  \param[out] r,g,b color values
  \param norm
  \param nval
  \param dflt

  \return 0 on failure
  \return 1 on success
*/
int Rast_read_color_rule(void *closure, DCELL min, DCELL max,
			 DCELL * val, int *r, int *g, int *b,
			 int *norm, int *nval, int *dflt)
{
    char buf[1024];
    FILE *fp = closure;
    int ret;

    *norm = *nval = *dflt = 0;

    for (;;) {
	if (!G_getl2(buf, sizeof(buf), fp))
	    return 0;

	G_strip(buf);
	G_debug(5, "color buf = [%s]", buf);

	if (*buf == '\0')
	    continue;
	if (*buf == '#')
	    continue;

	ret =
	    Rast_parse_color_rule(min, max, buf, val, r, g, b, norm, nval,
				  dflt);
	if (ret == 0)
	    return 1;

	G_fatal_error(_("bad rule (%s): [%s]"),
		      Rast_parse_color_rule_error(ret), buf);
    }

    return 0;
}

/*!
  \brief Read color rules from file
  
  \param[out] colors pointer to Colors structure
  \param min, max min & max values (used only when color rules are in percentage)
  \param read_rule pointer to read_rule_fn structure
  \param closure 

  \return 0 on failure
  \return 1 on success
*/
int Rast_read_color_rules(struct Colors *colors, DCELL min, DCELL max,
			  read_rule_fn * read_rule, void *closure)
{
    struct rule *rule = NULL;
    int nrules = 0;
    struct rule dflt, null;
    int set, is_null, is_dflt, r, g, b;
    DCELL val;
    int n;

    if (!read_rule)
	read_rule = Rast_read_color_rule;

    Rast_init_colors(colors);

    /* initialization */
    dflt.r = dflt.g = dflt.b = dflt.set = 0;
    null.r = null.g = null.b = null.set = 0;

    while ((*read_rule)
	   (closure, min, max, &val, &r, &g, &b, &set, &is_null, &is_dflt)) {
	struct rule *p = NULL;

	if (set) {
	    n = nrules++;
	    rule = G_realloc(rule, nrules * sizeof(struct rule));
	    p = &rule[n];
	}
	else if (is_dflt)
	    p = &dflt;
	else if (is_null)
	    p = &null;

	if (!p)
	    G_fatal_error(_("Unknown error reading color rule"));

	p->r = r;
	p->g = g;
	p->b = b;
	p->set = 1;
	p->val = val;
    }

    if (nrules == 0)
	return 0;

    if (nrules == 1) {
	const struct rule *p = &rule[0];

	Rast_set_d_color(p->val, p->r, p->g, p->b, colors);
    }

    for (n = 1; n < nrules; n++) {
	struct rule *lo = &rule[n - 1];
	struct rule *hi = &rule[n];

	Rast_add_d_color_rule(&lo->val, lo->r, lo->g, lo->b,
			      &hi->val, hi->r, hi->g, hi->b, colors);
    }

    G_free(rule);

    /* null value and default color set up, if rules are set up by user */
    if (null.set)
	Rast_set_null_value_color(null.r, null.g, null.b, colors);

    if (dflt.set)
	Rast_set_default_color(dflt.r, dflt.g, dflt.b, colors);

    return 1;
}

static int load_rules_file(struct Colors *colors, const char *path, DCELL min,
			   DCELL max)
{
    FILE *fp;
    int ret;

    fp = fopen(path, "r");

    if (!fp)
	return 0;

    ret =
	Rast_read_color_rules(colors, min, max, Rast_read_color_rule,
			      (void *)fp);

    fclose(fp);

    return ret;
}

/*!
  \brief Load color rules from file
  
  \param[out] colors pointer to Colors structure
  \param path path to the color rules file
  \param min, max min & max values (used only when color rules are in percentage)

  \return 0 on failure
  \return 1 on success
*/
int Rast_load_colors(struct Colors *colors, const char *path, CELL min,
		     CELL max)
{
    return load_rules_file(colors, path, (DCELL) min, (DCELL) max);
}

/*!
  \brief Load color floating-point rules from file
  
  \param[out] colors pointer to Colors structure
  \param path path to the color rules file
  \param min, max min & max values (used only when color rules are in percentage)

  \return 0 on failure
  \return 1 on success
*/
int Rast_load_fp_colors(struct Colors *colors, const char *path, DCELL min,
			DCELL max)
{
    return load_rules_file(colors, path, min, max);
}

static void load_rules_name(struct Colors *colors, const char *name,
			    DCELL min, DCELL max)
{
    char path[GPATH_MAX];

    sprintf(path, "%s/etc/colors/%s", G_gisbase(), name);

    if (!load_rules_file(colors, path, min, max))
	G_fatal_error(_("Unable to load color rules <%s>"), name);
}

/*!
  \brief Load color rules from predefined color table

  \param[out] colors pointer to Colors structure
  \param name name of color table to load
  \param min, max min & max values (used only when color rules are in percentage)
*/
void Rast_make_colors(struct Colors *colors, const char *name, CELL min,
		      CELL max)
{
    load_rules_name(colors, name, (DCELL) min, (DCELL) max);
}

/*!
  \brief Load color rules from predefined floating-point color table

  \param[out] colors pointer to Colors structure
  \param name name of color table to load
  \param min, max min & max values (used only when color rules are in percentage)
*/
void Rast_make_fp_colors(struct Colors *colors, const char *name, DCELL min,
			 DCELL max)
{
    load_rules_name(colors, name, min, max);
}
