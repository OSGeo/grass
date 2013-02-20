
/****************************************************************************
 *
 * MODULE:       i.ortho.transform   (cloned from m.transform and g.transform)
 * AUTHOR(S):    Brian J. Buckley
 *               Glynn Clements
 *               Hamish Bowman
 *               Markus Metz
 * PURPOSE:      Utility to compute transformation based upon GCPs and 
 *               output error measurements
 * COPYRIGHT:    (C) 2006-2013 by the GRASS Development Team
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
#include "orthophoto.h"

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

static int summary;
static int forward;
static char **columns;
static int need_fwd;
static int need_rev;
static int need_fd;
static int need_rd;
static char *coord_file;

struct Ortho_Image_Group group;

static struct Ortho_Control_Points *points;

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
    int n, i, status;
    double e0, e1, e2, n0, n1, n2, z1, z2;
    struct Ortho_Control_Points temp_points;

    /* compute photo <-> image equations */
    group.ref_equation_stat = I_compute_ref_equations(&group.photo_points,
						      group.E12, group.N12,
						      group.E21, group.N21);

    if (group.ref_equation_stat <= 0)
	G_fatal_error(_("Error conducting transform (%d)"),
	              group.ref_equation_stat);

    /* compute target <-> photo equations */

    /* alloc and fill temp control points */
    temp_points.count = 0;
    temp_points.status = NULL;
    temp_points.e1 = NULL;
    temp_points.n1 = NULL;
    temp_points.z1 = NULL;
    temp_points.e2 = NULL;
    temp_points.n2 = NULL;
    temp_points.z2 = NULL;

    /* e0, n0, equal photo coordinates not image coords */
    for (i = 0; i < group.control_points.count; i++) {
	status = group.control_points.status[i];
	e1 = group.control_points.e1[i];
	n1 = group.control_points.n1[i];
	z1 = group.control_points.z1[i];
	e2 = group.control_points.e2[i];
	n2 = group.control_points.n2[i];
	z2 = group.control_points.z2[i];

	/* image to photo transformation */
	I_georef(e1, n1, &e0, &n0, group.E12, group.N12, 1);
	I_new_con_point(&temp_points, e0, n0, z1, e2, n2, z2, status);
    }


    group.con_equation_stat = I_compute_ortho_equations(&temp_points,
							&group.camera_ref,
							&group.camera_exp,
							&group.XC, &group.YC,
							&group.ZC,
							&group.omega,
							&group.phi,
							&group.kappa,
							&group.M,
							&group.MI);

    if (group.con_equation_stat <= 0)
	G_fatal_error(_("Error conducting transform (%d)"),
	              group.con_equation_stat);

    count = 0;

    for (n = 0; n < points->count; n++) {
	double e1, n1, e2, n2;
	double fx, fy, fd, fd2;
	double rx, ry, rd, rd2;

	if (points->status[n] <= 0)
	    continue;

	count++;

	if (need_fwd) {
	    /* image -> photo -> target */

	    /* image coordinates ex, nx to photo coordinates ex1, nx1 */
	    I_georef(points->e1[n], points->n1[n], &e1, &n1, group.E12, group.N12, 1);

	    /* photo coordinates ex1, nx1 to target coordinates e1, n1 */
	    I_inverse_ortho_ref(e1, n1, points->z1[n], &e2, &n2, &z2,
	                        &group.camera_ref,
				group.XC, group.YC, group.ZC, group.MI);

	    fx = fabs(e2 - points->e2[n]);
	    fy = fabs(n2 - points->n2[n]);

	    if (need_fd)
		diagonal(&fd, &fd2, fx, fy);

	    if (summary)
		update_stats(&fwd, n, fx, fy, fd, fd2);
	}

	if (need_rev) {
	    /* target -> photo -> image */

	    /* target coordinates e1, n1 to photo coordinates ex1, nx1 */
	    I_ortho_ref(points->e2[n], points->n2[n], points->z2[n],
	                &e2, &n2, &z2, &group.camera_ref,
			group.XC, group.YC, group.ZC, group.M);

	    /* photo coordinates ex1, nx1 to image coordinates ex, nx */
	    I_georef(e2, n2, &e1, &n1, group.E21, group.N21, 1);

	    rx = fabs(e1 - points->e1[n]);
	    ry = fabs(n1 - points->n1[n]);

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
		printf(" %f %f", points->e1[n], points->n1[n]);
	    if (strcmp("dst", col) == 0)
		printf(" %f %f", points->e2[n], points->n2[n]);
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
    if (group.ref_equation_stat == -1)
	G_warning(_("Poorly placed image to photo control points"));
    else if (group.con_equation_stat == -1)
	G_warning(_("Poorly placed image to target control points"));
    else if (group.ref_equation_stat == -2 || group.con_equation_stat == -2)
	G_fatal_error(_("Insufficient memory"));
    else if (group.ref_equation_stat < 0 || group.con_equation_stat < 0)
	G_fatal_error(_("Parameter error"));
    else if (group.ref_equation_stat == 0 || group.con_equation_stat == 0)
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

    for (i = 0; i < 3; i++)
    	fprintf(stdout, "E%d=%.15g\n", i, forward ? group.E12[i] : group.E21[i]);

    for (i = 0; i < 3; i++)
    	fprintf(stdout, "N%d=%.15g\n", i, forward ? group.N12[i] : group.N21[i]);

    /* print ortho transformation matrix ? */
}

static void xform_value(double east, double north, double height)
{
    double e1, n1, z1, xe, xn, xz;

    if (forward) {
	/* image -> photo -> target */
	I_georef(east, north, &e1, &n1, group.E12, group.N12, 1);
	z1 = height;
	I_inverse_ortho_ref(e1, n1, z1, &xe, &xn, &xz, &group.camera_ref,
		    group.XC, group.YC, group.ZC, group.MI);
	xz = z1;
    }
    else {
	/* target -> photo -> image */
	I_ortho_ref(east, north, height, &e1, &n1, &z1, &group.camera_ref,
		    group.XC, group.YC, group.ZC, group.M);

	I_georef(e1, n1, &xe, &xn, group.E21, group.N21, 1);
	xz = 0.;
    }

    fprintf(stdout, "%.15g %.15g %.15g\n", xe, xn, xz);
}

static void do_pt_xforms(void)
{
    double easting, northing, height;
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
    	char buf[1024];

    	if (!G_getl2(buf, sizeof(buf), fp))
    	    break;

    	if ((buf[0] == '#') || (buf[0] == '\0'))
    	    continue;

    	/* ? sscanf(buf, "%s %s", &east_str, &north_str)
    	    ? G_scan_easting(,,-1)
    	    ? G_scan_northing(,,-1) */
    	/* ? muliple delims with sscanf(buf, "%[ ,|\t]", &dummy) ? */

    	ret = sscanf(buf, "%lf %lf %lf", &easting, &northing, &height);
    	if (ret != 3)
    	    G_fatal_error(_("Invalid coordinates: [%s]"), buf);

    	xform_value(easting, northing, height);
    }

    if (fp != stdin)
    	fclose(fp);
}


int main(int argc, char **argv)
{
    struct Option *grp, *fmt, *xfm_pts;
    struct Flag *sum, *rev_flag, *dump_flag;
    struct GModule *module;
    char *desc;

    G_gisinit(argv[0]);

    /* Get Args */
    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("orthorectify"));
    G_add_keyword(_("transformation"));
    G_add_keyword(_("GCP"));
    module->description =
	_("Computes a coordinate transformation based on the control points.");

    grp = G_define_standard_option(G_OPT_I_GROUP);

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


    G_strip(grp->answer);
    strcpy(group.name, grp->answer);

    summary = !!sum->answer;
    columns = fmt->answers;
    forward = !rev_flag->answer;
    coord_file = xfm_pts->answer;

    if (!I_get_ref_points(group.name, &group.photo_points)) {
	G_fatal_error(_("Can not read reference points for group <%s>"),
	              group.name);
    }
    if (!I_get_con_points(group.name, &group.control_points)) {
	G_fatal_error(_("Can not read control points for group <%s>"),
	              group.name);
    }
    
    points = &group.control_points;

    parse_format();

    compute_transformation();

    analyze();

    if (dump_flag->answer)
	dump_cooefs();

    if (coord_file)
	do_pt_xforms();

    return 0;
}
