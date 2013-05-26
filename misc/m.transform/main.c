
/****************************************************************************
 *
 * MODULE:       m.transform   (nee g.transform)
 * AUTHOR(S):    Brian J. Buckley
 *               Glynn Clements
 *               Hamish Bowman
 * PURPOSE:      Utility to compute transformation based upon GCPs and 
 *               output error measurements
 * COPYRIGHT:    (C) 2006-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

struct Max
{
    int idx;
    double val;
};

struct Stats
{
    struct Max x, y, g;
    double sum2, rms;
};

static char *name;
static int order;
static int summary;
static int forward;
static char **columns;
static int need_fwd;
static int need_rev;
static int need_fd;
static int need_rd;
static char *coord_file;

static double E12[10], N12[10], E21[10], N21[10];

static struct Control_Points points;

static int equation_stat;

static int count;
static struct Stats fwd, rev;

static void update_max(struct Max *m, int n, double k)
{
    if (k > m->val) {
	m->idx = n;
	m->val = k;
    }
}

static void update_stats(struct Stats *st, int n, double dx, double dy,
			 double dg, double d2)
{
    update_max(&st->x, n, dx);
    update_max(&st->y, n, dy);
    update_max(&st->g, n, dg);
    st->sum2 += d2;
}

static void diagonal(double *dg, double *d2, double dx, double dy)
{
    *d2 = dx * dx + dy * dy;
    *dg = sqrt(*d2);
}

static void compute_transformation(void)
{
    static const int order_pnts[3] = { 3, 6, 10 };
    int n, i;

    equation_stat =
	I_compute_georef_equations(&points, E12, N12, E21, N21, order);

    if (equation_stat == 0)
	G_fatal_error(_("Not enough points, %d are required"),
		      order_pnts[order - 1]);

    if (equation_stat <= 0)
	G_fatal_error(_("Error conducting transform (%d)"), equation_stat);

    count = 0;

    for (n = 0; n < points.count; n++) {
	double e1, n1, e2, n2;
	double fx, fy, fd, fd2;
	double rx, ry, rd, rd2;

	if (points.status[n] <= 0)
	    continue;

	count++;

	if (need_fwd) {
	    I_georef(points.e1[n], points.n1[n], &e2, &n2, E12, N12, order);

	    fx = fabs(e2 - points.e2[n]);
	    fy = fabs(n2 - points.n2[n]);

	    if (need_fd)
		diagonal(&fd, &fd2, fx, fy);

	    if (summary)
		update_stats(&fwd, n, fx, fy, fd, fd2);
	}

	if (need_rev) {
	    I_georef(points.e2[n], points.n2[n], &e1, &n1, E21, N21, order);

	    rx = fabs(e1 - points.e1[n]);
	    ry = fabs(n1 - points.n1[n]);

	    if (need_rd)
		diagonal(&rd, &rd2, rx, ry);

	    if (summary)
		update_stats(&rev, n, rx, ry, rd, rd2);
	}

	if (!columns[0])
	    continue;

	if (coord_file)
	    continue;

	for (i = 0;; i++) {
	    const char *col = columns[i];

	    if (!col)
		break;

	    if (strcmp("idx", col) == 0)
		printf(" %d", n);
	    if (strcmp("src", col) == 0)
		printf(" %f %f", points.e1[n], points.n1[n]);
	    if (strcmp("dst", col) == 0)
		printf(" %f %f", points.e2[n], points.n2[n]);
	    if (strcmp("fwd", col) == 0)
		printf(" %f %f", e2, n2);
	    if (strcmp("rev", col) == 0)
		printf(" %f %f", e1, n1);
	    if (strcmp("fxy", col) == 0)
		printf(" %f %f", fx, fy);
	    if (strcmp("rxy", col) == 0)
		printf(" %f %f", rx, ry);
	    if (strcmp("fd", col) == 0)
		printf(" %f", fd);
	    if (strcmp("rd", col) == 0)
		printf(" %f", rd);
	}

	printf("\n");
    }

    if (summary && count > 0) {
	fwd.rms = sqrt(fwd.sum2 / count);
	rev.rms = sqrt(rev.sum2 / count);
    }
}

static void do_max(char name, const struct Max *m)
{
    printf("%c[%d] = %.2f\n", name, m->idx, m->val);
}

static void do_stats(const char *name, const struct Stats *st)
{
    printf("%s:\n", name);
    do_max('x', &st->x);
    do_max('y', &st->y);
    do_max('g', &st->g);
    printf("RMS = %.2f\n", st->rms);
}

static void analyze(void)
{
    if (equation_stat == -1)
	G_warning(_("Poorly placed control points"));
    else if (equation_stat == -2)
	G_fatal_error(_("Insufficient memory"));
    else if (equation_stat < 0)
	G_fatal_error(_("Parameter error"));
    else if (equation_stat == 0)
	G_fatal_error(_("No active control points"));
    else if (summary) {
	printf("Number of active points: %d\n", count);
	do_stats("Forward", &fwd);
	do_stats("Reverse", &rev);
    }
}

static void parse_format(void)
{
    int i;

    if (summary) {
	need_fwd = need_rev = need_fd = need_rd = 1;
	return;
    }

    if (!columns)
	return;

    for (i = 0;; i++) {
	const char *col = columns[i];

	if (!col)
	    break;

	if (strcmp("fwd", col) == 0)
	    need_fwd = 1;
	if (strcmp("fxy", col) == 0)
	    need_fwd = 1;
	if (strcmp("fd", col) == 0)
	    need_fwd = need_fd = 1;
	if (strcmp("rev", col) == 0)
	    need_rev = 1;
	if (strcmp("rxy", col) == 0)
	    need_rev = 1;
	if (strcmp("rd", col) == 0)
	    need_rev = need_rd = 1;
    }
}

static void dump_cooefs(void)
{
    int i;
    static const int order_pnts[3] = { 3, 6, 10 };

    for (i = 0; i < order_pnts[order - 1]; i++)
    	fprintf(stdout, "E%d=%.15g\n", i, forward ? E12[i] : E21[i]);

    for (i = 0; i < order_pnts[order - 1]; i++)
    	fprintf(stdout, "N%d=%.15g\n", i, forward ? N12[i] : N21[i]);
}

static void xform_value(double east, double north)
{
    double xe, xn;

    if(forward)
	I_georef(east, north, &xe, &xn, E12, N12, order);
    else
	I_georef(east, north, &xe, &xn, E21, N21, order);

    fprintf(stdout, "%.15g %.15g\n", xe, xn);
}

static void do_pt_xforms(void)
{
    double easting, northing;
    int ret;
    FILE *fp;

    if (strcmp(coord_file, "-") == 0)
    	fp = stdin;
    else {
    	fp = fopen(coord_file, "r");
    	if (!fp)
    	    G_fatal_error(_("Unable to open file <%s>"), coord_file);
    }

    for (;;) {
    	char buf[64];

    	if (!G_getl2(buf, sizeof(buf), fp))
    	    break;

    	if ((buf[0] == '#') || (buf[0] == '\0'))
    	    continue;

    	/* ? sscanf(buf, "%s %s", &east_str, &north_str)
    	    ? G_scan_easting(,,-1)
    	    ? G_scan_northing(,,-1) */
    	/* ? muliple delims with sscanf(buf, "%[ ,|\t]", &dummy) ? */

    	ret = sscanf(buf, "%lf %lf", &easting, &northing);
    	if (ret != 2)
    	    G_fatal_error(_("Invalid coordinates: [%s]"), buf);

    	xform_value(easting, northing);
    }

    if (fp != stdin)
    	fclose(fp);
}


int main(int argc, char **argv)
{
    struct Option *grp, *val, *fmt, *xfm_pts;
    struct Flag *sum, *rev_flag, *dump_flag;
    struct GModule *module;
    char *desc;

    G_gisinit(argv[0]);

    /* Get Args */
    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("transformation"));
    G_add_keyword("GCP");
    module->description =
	_("Computes a coordinate transformation based on the control points.");

    grp = G_define_standard_option(G_OPT_I_GROUP);

    val = G_define_option();
    val->key = "order";
    val->type = TYPE_INTEGER;
    val->required = YES;
    val->options = "1-3";
    val->description = _("Rectification polynomial order");

    fmt = G_define_option();
    fmt->key = "format";
    fmt->type = TYPE_STRING;
    fmt->required = NO;
    fmt->multiple = YES;
    fmt->options = "idx,src,dst,fwd,rev,fxy,rxy,fd,rd";
    desc = NULL;
    G_asprintf(&desc,
	        "idx;%s;src;%s;dst;%s;fwd;%s;rev;%s;fxy;%s;rxy;%s;fd;%s;rd;%s",
	        _("point index"),
	        _("source coordinates"),
	        _("destination coordinates"),
	        _("forward coordinates (destination)"),
	        _("reverse coordinates (source)"),
	        _("forward coordinates difference (destination)"),
	        _("reverse coordinates difference (source)"),
	        _("forward error (destination)"),
	        _("reverse error (source)"));
    fmt->descriptions = desc;
    fmt->answer = "fd,rd";
    fmt->description = _("Output format");

    sum = G_define_flag();
    sum->key = 's';
    sum->description = _("Display summary information");

    xfm_pts = G_define_standard_option(G_OPT_F_INPUT);
    xfm_pts->key = "coords";
    xfm_pts->required = NO;
    xfm_pts->label =
	_("File containing coordinates to transform (\"-\" to read from stdin)");
    xfm_pts->description = _("Local x,y coordinates to target east,north");

    rev_flag = G_define_flag();
    rev_flag->key = 'r';
    rev_flag->label = _("Reverse transform of coords file or coeff. dump");
    rev_flag->description = _("Target east,north coordinates to local x,y");

    dump_flag = G_define_flag();
    dump_flag->key = 'x';
    dump_flag->description = _("Display transform matrix coefficients");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    name = grp->answer;
    order = atoi(val->answer);
    summary = !!sum->answer;
    columns = fmt->answers;
    forward = !rev_flag->answer;
    coord_file = xfm_pts->answer;

    I_get_control_points(name, &points);

    parse_format();

    compute_transformation();

    I_put_control_points(name, &points);

    analyze();

    if(dump_flag->answer)
	dump_cooefs();

    if(coord_file)
	do_pt_xforms();

    return 0;
}
