/****************************************************************************
 *
 * MODULE:       gis library
 * AUTHOR(S):    Glynn Clements <glynn@gclements.plus.com>
 * COPYRIGHT:    (C) 2007 Glynn Clements and the GRASS Development Team
 *
 * NOTE:         Based upon r.colors/rules.c
 *               The colors are stored in ./colors/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>

struct rule
{
    int set;
    int r,g,b;
    DCELL val;
};

enum rule_error
{
    CR_OK = 0,
    CR_ERROR_SYNTAX,
    CR_ERROR_RGB,
    CR_ERROR_COLOR,
    CR_ERROR_PERCENT,
    CR_ERROR_VALUE,
};

int G_parse_color_rule(
    DCELL min, DCELL max, const char *buf,
    DCELL *val, int *r, int *g, int *b,
    int *norm, int *nval, int *dflt)
{
    char value[80], color[80];
    double x;
    char c;

    *norm = *nval = *dflt = 0;

    if (sscanf(buf, "%s %[^\n]", value, color) != 2)
	return CR_ERROR_SYNTAX;

    G_chop(color);

    if (sscanf(color, "%d:%d:%d", r, g, b) == 3 ||
	sscanf(color, "%d %d %d", r, g, b) == 3)
    {
	if (*r<0 || *r>255 || *g<0 || *g>255 || *b<0 || *b>255)
	    return CR_ERROR_RGB;
    }
    else
    {
	float fr, fg, fb;

	if (G_color_values(color, &fr, &fg, &fb) < 0)
	    return CR_ERROR_COLOR;

	*r = (int) (fr * 255.99);
	*g = (int) (fg * 255.99);
	*b = (int) (fb * 255.99);
    }

    G_chop(value);

    if (G_strcasecmp(value, "default") == 0)
    {
	*dflt = 1;
	return CR_OK;
    }

    if (G_strcasecmp(value, "nv") == 0)
    {
	*nval = 1;
	return CR_OK;
    }

    if (sscanf(value, "%lf%c", &x, &c) == 2 && c == '%')
    {
	if (x < 0 || x > 100)
	    return CR_ERROR_PERCENT;

	*val = min + (max - min) * (x / 100);
	*norm = 1;
	return CR_OK;
    }

    if (sscanf(value, "%lf", val) == 1)
    {
	*norm = 1;
	return CR_OK;
    }

    return CR_ERROR_VALUE;
}

const char *G_parse_color_rule_error(int code)
{
    switch (code)
    {
    case CR_OK:			return "";
    case CR_ERROR_SYNTAX:	return _("syntax error");
    case CR_ERROR_RGB:		return _("R/G/B not in range 0-255");
    case CR_ERROR_COLOR:	return _("invalid color name");
    case CR_ERROR_PERCENT:	return _("percentage not in range 0-100");
    case CR_ERROR_VALUE:	return _("invalid value");
    default:			return _("unknown error");
    }
}

int G_read_color_rule(
    void *closure, DCELL min, DCELL max,
    DCELL *val, int *r, int *g, int *b,
    int *norm, int *nval, int *dflt)
{
    char buf[1024];
    FILE *fp = closure;
    int ret;

    *norm = *nval = *dflt = 0;

    for (;;)
    {
	if (!G_getl2(buf, sizeof(buf), fp))
	    return 0;

	G_strip(buf);
	G_debug(5, "color buf = [%s]", buf);

	if (*buf == '\0')
	    continue;
	if (*buf == '#')
	    continue;

	ret = G_parse_color_rule(min, max, buf, val, r, g, b, norm, nval, dflt);
	if (ret == 0)
	    return 1;

	G_fatal_error(_("bad rule (%s): [%s]"),
	     G_parse_color_rule_error(ret), buf);
    }

    return 0;
}

int G_read_color_rules(struct Colors *colors, DCELL min, DCELL max, read_rule_fn *read_rule, void *closure)
{
    struct rule *rule = NULL;
    int nrules = 0;
    struct rule dflt, null;
    int set, is_null, is_dflt, r, g, b;
    DCELL val;
    int n;

    if (!read_rule)
	read_rule = G_read_color_rule;

    G_init_colors(colors);

    /* initialization */
    dflt.r = dflt.g = dflt.b = dflt.set = 0;
    null.r = null.g = null.b = null.set = 0;

    while ((*read_rule)(closure, min, max, &val, &r, &g, &b, &set, &is_null, &is_dflt))
    {
	struct rule *p;

	if (set)
	{
	    n = nrules++;
	    rule = G_realloc(rule, nrules * sizeof(struct rule));
	    p = &rule[n];
	}
	else if (is_dflt)
	    p = &dflt;
	else if (is_null)
	    p = &null;

	p->r = r;
	p->g = g;
	p->b = b;
	p->set = 1;
	p->val = val;
    }

    if (nrules == 0)
	return 0;

    if (nrules == 1)
    {
	const struct rule *p = &rule[0];

	G_set_d_color(p->val, p->r, p->g, p->b, colors);
    }

    for (n = 1; n < nrules; n++)
    {
	struct rule *lo = &rule[n-1];
	struct rule *hi = &rule[n];

	G_add_d_raster_color_rule (
	    &lo->val, lo->r, lo->g, lo->b,
	    &hi->val, hi->r, hi->g, hi->b,
	    colors);
    }

    /* null value and default color set up, if rules are set up by user */
    if (null.set)
	G_set_null_value_color(null.r, null.g, null.b, colors);

    if (dflt.set)
    	G_set_default_color(dflt.r, dflt.g, dflt.b, colors);

    return 1;
}

static int load_rules_file(struct Colors *colors, const char *path, DCELL min, DCELL max)
{
    FILE *fp;
    int ret;

    fp = fopen(path, "r");

    if (!fp)
	return 0;

    ret = G_read_color_rules(colors, min, max, G_read_color_rule, (void *) fp);

    fclose(fp);

    return ret;
}

int G_load_colors(struct Colors *colors, const char *path, CELL min, CELL max)
{
    return load_rules_file(colors, path, (DCELL) min, (DCELL) max);
}

int G_load_fp_colors(struct Colors *colors, const char *path, DCELL min, DCELL max)
{
    return load_rules_file(colors, path, min, max);
}

static int load_rules_name(struct Colors *colors, const char *name, DCELL min, DCELL max)
{
    char path[GPATH_MAX];

    sprintf(path, "%s/etc/colors/%s", G_gisbase(), name);

    return load_rules_file(colors, path, min, max);
}

int G_make_colors(struct Colors *colors, const char *name, CELL min, CELL max)
{
    return load_rules_name(colors, name, (DCELL) min, (DCELL) max);
}

int G_make_fp_colors(struct Colors *colors, const char *name, DCELL min, DCELL max)
{
    return load_rules_name(colors, name, min, max);
}

