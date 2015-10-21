#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "global.h"
#include "crs.h"

struct Stats
{
    double x, y, z, g;
    double sum2, rms;
};

static void update_stats(struct Stats *st, int n,
                         double dx, double dy, double *dz,
			 double dg, double d2)
{
    st->x += dx;
    st->y += dy;
    if (dz)
	st->z += *dz;
    st->g += dg;
    st->sum2 += d2;
}

static void diagonal(double *dg, double *d2, double dx, double dy, double *dz)
{
    *d2 = dx * dx + dy * dy;
    if (dz)
	*d2 += *dz * *dz;
    *dg = sqrt(*d2);
}


static void compute_rms(struct Control_Points *cp, struct Control_Points_3D *cp3,
                        int order, int use3d, int orthorot, char *sep, FILE *fp)
{
    int n;
    int count, npoints;
    struct Stats fwd, rev;

    fwd.sum2 = fwd.rms = rev.sum2 = rev.rms = 0.0;
    
    fwd.x = fwd.y = fwd.z = fwd.g = 0.0;
    rev.x = rev.y = rev.z = rev.g = 0.0;

    count = 0;
    
    /* print index, forward difference, backward difference, 
     * forward rms, backward rms */
    if (use3d)
	fprintf(fp, "index%sfwd_dx%sfwd_dy%sfwd_dz%sback_dx%sback_dy%sback_dz%sfwd_RMS%sback_RMS",
               sep, sep, sep, sep, sep, sep, sep, sep);
    else
	fprintf(fp, "index%sfwd_dx%sfwd_dy%sback_dx%sback_dy%sfwd_RMS%sback_RMS",
	       sep, sep, sep, sep, sep, sep);

    fprintf(fp, "\n");
    
    if (use3d)
	npoints = cp3->count;
    else
	npoints = cp->count;

    for (n = 0; n < npoints; n++) {
	double e1, n1, z1, e2, n2, z2;
	double fx, fy, fz, fd, fd2;
	double rx, ry, rz, rd, rd2;

	if (use3d || orthorot) {
	    if (cp3->status[n] <= 0)
		continue;
	}
	else {
	    if (cp->status[n] <= 0)
		continue;
	}

	count++;

	/* forward: source -> target */
	if (use3d) {
	    if (orthorot)
		CRS_georef_or(cp3->e1[n], cp3->n1[n], cp3->z1[n],
			      &e2, &n2, &z2, OR12);
	    else
		CRS_georef_3d(cp3->e1[n], cp3->n1[n], cp3->z1[n],
			      &e2, &n2, &z2, 
			      E12, N12, Z12, 
			      order);

	    fx = fabs(e2 - cp3->e2[n]);
	    fy = fabs(n2 - cp3->n2[n]);
	    fz = fabs(z2 - cp3->z2[n]);

	    diagonal(&fd, &fd2, fx, fy, &fz);

	    update_stats(&fwd, n, fx, fy, &fz, fd, fd2);
	}
	else {
	    I_georef(cp->e1[n], cp->n1[n], &e2, &n2, E12, N12, order);

	    fx = fabs(e2 - cp->e2[n]);
	    fy = fabs(n2 - cp->n2[n]);

	    diagonal(&fd, &fd2, fx, fy, NULL);

	    update_stats(&fwd, n, fx, fy, NULL, fd, fd2);
	}

	/* backward: target -> source */
	if (use3d) {
	    if (orthorot)
		CRS_georef_or(cp3->e2[n], cp3->n2[n], cp3->z2[n],
			      &e1, &n1, &z1, OR21);
	    else
		CRS_georef_3d(cp3->e2[n], cp3->n2[n], cp3->z2[n],
			      &e1, &n1, &z1,
			      E21, N21, Z21,
			      order);

	    rx = fabs(e1 - cp3->e1[n]);
	    ry = fabs(n1 - cp3->n1[n]);
	    rz = fabs(z1 - cp3->z1[n]);

	    diagonal(&rd, &rd2, rx, ry, &rz);

	    update_stats(&rev, n, rx, ry, &rz, rd, rd2);
	}
	else {
	    I_georef(cp->e2[n], cp->n2[n], &e1, &n1, E21, N21, order);

	    rx = fabs(e1 - cp->e1[n]);
	    ry = fabs(n1 - cp->n1[n]);

	    diagonal(&rd, &rd2, rx, ry, NULL);

	    update_stats(&rev, n, rx, ry, NULL, rd, rd2);
	}

	/* print index, forward difference, backward difference, 
	 * forward rms, backward rms */
	fprintf(fp, "%d", n + 1);
	fprintf(fp, "%s%f%s%f", sep, fx, sep, fy);
	if (use3d)
	    fprintf(fp, "%s%f", sep, fz);
	fprintf(fp, "%s%f%s%f", sep, rx, sep, ry);
	if (use3d)
	    fprintf(fp, "%s%f", sep, rz);
	fprintf(fp, "%s%.4f", sep, fd);
	fprintf(fp, "%s%.4f", sep, rd);

	fprintf(fp, "\n");
    }

    if (count > 0) {
	fwd.x /= count;
	fwd.y /= count;
	fwd.g /= count;
	rev.x /= count;
	rev.y /= count;
	rev.g /= count;
	if (use3d) {
	    fwd.z /= count;
	    rev.z /= count;
	}
	fwd.rms = sqrt(fwd.sum2 / count);
	rev.rms = sqrt(rev.sum2 / count);
    }
    fprintf(fp, "%d", count);
    fprintf(fp, "%s%f%s%f", sep, fwd.x, sep, fwd.y);
    if (use3d)
	fprintf(fp, "%s%f", sep, fwd.z);
    fprintf(fp, "%s%f%s%f", sep, rev.x, sep, rev.y);
    if (use3d)
	fprintf(fp, "%s%f", sep, rev.z);
    fprintf(fp, "%s%.4f", sep, fwd.rms);
    fprintf(fp, "%s%.4f", sep, rev.rms);

    fprintf(fp, "\n");
}


int new_control_point_3d(struct Control_Points_3D *cp,
			double e1, double n1, double z1,
			double e2, double n2, double z2,
			int status)
{
    int i;
    unsigned int size;

    if (status < 0)
	return 1;

    i = (cp->count)++;
    size = cp->count * sizeof(double);
    cp->e1 = (double *)G_realloc(cp->e1, size);
    cp->e2 = (double *)G_realloc(cp->e2, size);
    cp->n1 = (double *)G_realloc(cp->n1, size);
    cp->n2 = (double *)G_realloc(cp->n2, size);
    cp->z1 = (double *)G_realloc(cp->z1, size);
    cp->z2 = (double *)G_realloc(cp->z2, size);
    size = cp->count * sizeof(int);
    cp->status = (int *)G_realloc(cp->status, size);

    cp->e1[i] = e1;
    cp->e2[i] = e2;
    cp->n1[i] = n1;
    cp->n2[i] = n2;
    cp->z1[i] = z1;
    cp->z2[i] = z2;
    cp->status[i] = status;

    return 0;
}

static int read_control_points(FILE * fd, struct Control_Points *cp)
{
    char buf[1000];
    double e1, e2, n1, n2;
    int status;

    cp->count = 0;

    /* read the control point lines. format is:
       image_east image_north  target_east target_north  status
     */
    cp->e1 = NULL;
    cp->e2 = NULL;
    cp->n1 = NULL;
    cp->n2 = NULL;
    cp->status = NULL;

    while (G_getl2(buf, sizeof buf, fd)) {
	G_strip(buf);
	if (*buf == '#' || *buf == 0)
	    continue;
	if (sscanf(buf, "%lf%lf%lf%lf%d", &e1, &n1, &e2, &n2, &status) == 5)
	    I_new_control_point(cp, e1, n1, e2, n2, status);
	else
	    return -4;
    }

    return 1;
}

static int read_control_points_3d(FILE * fd, struct Control_Points_3D *cp)
{
    char buf[1000];
    double e1, e2, n1, n2, z1, z2;
    int status;

    cp->count = 0;

    /* read the control point lines. format is:
       source_east source_north source_height  target_east target_north target_height  status
     */
    cp->e1 = NULL;
    cp->e2 = NULL;
    cp->n1 = NULL;
    cp->n2 = NULL;
    cp->z1 = NULL;
    cp->z2 = NULL;
    cp->status = NULL;

    while (G_getl2(buf, sizeof buf, fd)) {
	G_strip(buf);
	if (*buf == '#' || *buf == 0)
	    continue;
	if (sscanf(buf, "%lf%lf%lf%lf%lf%lf%d", &e1, &n1, &z1, &e2, &n2, &z2, &status) == 7)
	    new_control_point_3d(cp, e1, n1, z1, e2, n2, z2, status);
	else
	    return -4;
    }

    return 1;
}


int get_control_points(char *group, char *pfile, int order, int use3d, 
                       int orthorot, int rms, char *sep, FILE *fpr)
{
    char msg[200];
    struct Control_Points cp;
    struct Control_Points_3D cp3;
    int ret = 0;
    int order_pnts[2][3] = {{ 3, 6, 10 }, { 4, 10, 20 }};
    
    cp.count = cp3.count = 0;
    cp.e1 = cp.e2 = cp3.e1 = cp3.e2 = NULL;
    cp.n1 = cp.n2 = cp3.n1 = cp3.n2 = NULL;
    cp3.z1 = cp3.z2 = NULL;
    cp.status = cp3.status = NULL;
    
    msg[0] = '\0';

    if (use3d) {
	/* read 3D GCPs from points file */
	FILE *fp;
	int fd, stat;
	
	if ((fd = open(pfile, 0)) < 0)
	    G_fatal_error(_("Can not open file <%s>"), pfile);
	    
	fp = fdopen(fd, "r");

	stat = read_control_points_3d(fp, &cp3);
	fclose(fp);
	if (stat < 0) {
	    G_fatal_error(_("Bad format in control point file <%s>"),
		      pfile);
	    return 0;
	}

	if (orthorot)
	    ret = CRS_compute_georef_equations_or(&cp3, OR12, OR21);
	else
	    ret = CRS_compute_georef_equations_3d(&cp3, E12, N12, Z12,
	                                          E21, N21, Z21, order);
    }
    else if (pfile) {
	/* read 2D GCPs from points file */
	FILE *fp;
	int fd, stat;
	
	if ((fd = open(pfile, 0)) < 0)
	    G_fatal_error(_("Can not open file <%s>"), pfile);
	    
	fp = fdopen(fd, "r");

	stat = read_control_points(fp, &cp);
	fclose(fp);
	if (stat < 0) {
	    G_fatal_error(_("Bad format in control point file <%s>"),
		      pfile);
	    return 0;
	}

	ret = I_compute_georef_equations(&cp, E12, N12, E21, N21, order);
    }
    else {
	/* read group control points */
	if (!I_get_control_points(group, &cp))
	    exit(0);

	sprintf(msg, _("Control Point file for group <%s@%s> - "),
		group, G_mapset());
		
	ret = I_compute_georef_equations(&cp, E12, N12, E21, N21, order);
    }

    switch (ret) {
    case 0:
	sprintf(&msg[strlen(msg)],
		_("Not enough active control points for current order, %d are required."),
		(orthorot ? 3 : order_pnts[use3d != 0][order - 1]));
	break;
    case -1:
	strcat(msg, _("Poorly placed control points."));
	strcat(msg, _(" Can not generate the transformation equation."));
	break;
    case -2:
	strcat(msg, _("Not enough memory to solve for transformation equation"));
	break;
    case -3:
	strcat(msg, _("Invalid order"));
	break;
    default:
	break;
    }
    if (ret != 1)
        G_fatal_error("%s", msg);
	
    if (rms) {
	compute_rms(&cp, &cp3, order, use3d, orthorot, sep, fpr);
    }
    
    return 1;
}
