
/****************************************************************************
 *
 * MODULE:       r.colors
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *               David Johnson
 *
 * PURPOSE:      Allows creation and/or modification of the color table
 *               for a raster map layer.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "local_proto.h"

/* color structure for default and null value */

static int read_rule(void *, DCELL, DCELL, DCELL *, int *, int *, int *,
		     int *, int *, int *);
static void badrule(int, const char *, int);
static int show_colors(FILE *);

int read_color_rules(FILE * fp, struct Colors *colors, DCELL min, DCELL max,
		     int is_fp)
{
    DCELL rulemin, rulemax;

    if (isatty(fileno(fp))) {
	fprintf(stderr,
		_("Enter rules, \"end\" when done, \"help\" if you need it.\n"));

	if (is_fp) {
	    char minstr[64], maxstr[64];

	    sprintf(minstr, "%.15g", (double)min);
	    sprintf(maxstr, "%.15g", (double)max);
	    G_trim_decimal(minstr);
	    G_trim_decimal(maxstr);
	    fprintf(stderr, _("fp: Data range is %s to %s\n"), minstr,
		    maxstr);
	}
	else
	    fprintf(stderr, _("Data range is %ld to %ld\n"), (long)min,
		    (long)max);
    }

    if (!Rast_read_color_rules(colors, min, max, read_rule, fp))
	return 0;

    Rast_get_d_color_range(&rulemin, &rulemax, colors);
    G_debug(3, "rulemin=%.3f  rulemax=%.3f", rulemin, rulemax);

    if (rulemin > min || rulemax < max)
	G_warning(_("Your color rules do not cover the whole range of data!\n (rules %f to %f but data %f to %f)"),
		  rulemin, rulemax, min, max);

    return 1;
}

static int read_rule(void *closure, DCELL min, DCELL max,
		     DCELL * val, int *r, int *g, int *b,
		     int *norm, int *nval, int *dflt)
{
    FILE *fp = closure;
    int tty = isatty(fileno(fp));

    *norm = *nval = *dflt = 0;

    for (;;) {
	char buf[1024];
	int ret, i;

	if (tty)
	    fprintf(stderr, "> ");

	if (!G_getl2(buf, sizeof(buf), fp))
	    return 0;

	for (i = 0; buf[i]; i++)
	    if (buf[i] == ',')
		buf[i] = ' ';

	G_strip(buf);

	if (*buf == '\0')
	    continue;
	if (*buf == '#')
	    continue;

	if (strncmp(buf, "end", 3) == 0)
	    return 0;

	if (strncmp(buf, "help", 4) == 0) {
	    fprintf(stderr, _("Enter a rule in one of these formats:\n"));
	    fprintf(stderr, _(" val color\n"));
	    fprintf(stderr, _(" n%% color\n"));
	    fprintf(stderr, _(" nv color\n"));
	    fprintf(stderr, _(" default color\n"));
	    fprintf(stderr, _("color can be one of:\n"));
	    show_colors(stderr);
	    fprintf(stderr, _("or an R:G:B triplet, e.g.: 0:127:255\n"));

	    continue;
	}

	ret =
	    Rast_parse_color_rule(min, max, buf, val, r, g, b, norm, nval, dflt);
	if (ret == 0)
	    return 1;

	badrule(tty, buf, ret);
    }

    return 0;
}

static void badrule(int tty, const char *s, int code)
{
    const char *err = Rast_parse_color_rule_error(code);

    if (tty)
	G_warning(_("bad rule (%s); rule not added"), err);
    else
	G_fatal_error(_("bad rule (%s): [%s]"), err, s);
}

static int show_colors(FILE * fp)
{
    int len;
    int i, n;
    const char *color;

    len = 0;
    for (i = 0; (color = G_color_name(i)); i++) {
	n = strlen(color) + 1;
	if (len + n > 78) {
	    fprintf(fp, "\n");
	    len = 0;
	}
	fprintf(fp, " %s", color);
	len += n;
    }
    fprintf(fp, "\n");

    return 0;
}
