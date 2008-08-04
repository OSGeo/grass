
/**
 * \file icon.c
 *
 * \brief GIS Library - Plot icon
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>

static void trans(double *x, double *y, int n_points,
		  double angle, double scale, double xc, double yc)
{
    double m[2][2];
    double sin_a = sin(angle);
    double cos_a = cos(angle);
    int i;

    m[0][0] = cos_a * scale;
    m[0][1] = -sin_a * scale;
    m[1][0] = sin_a * scale;
    m[1][1] = cos_a * scale;

    for (i = 0; i < n_points; i++) {
	double xi = x[i];
	double yi = y[i];

	x[i] = m[0][0] * xi + m[0][1] * yi + xc;
	y[i] = m[1][0] * xi + m[1][1] * yi + yc;
    }
}

/**
 * \brief Plot icon
 *
 * \param[in] xc,yc icon coordinates
 * \param[in] type  icon type
 * \param[in] angle rotation angle [rad]
 * \param[in] scale scale factor
 *
 * \return 1
 */
int G_plot_icon(double xc, double yc, int type, double angle, double scale)
{
    int i, np = 0;
    double x[10], y[10];

    G_debug(2, "G_plot_icon(): xc=%g, yc=%g", xc, yc);

    /* diamond, box */
    switch (type) {
    case G_ICON_CROSS:
	x[0] = -0.5;
	y[0] = 0.0;
	x[1] = 0.5;
	y[1] = 0.0;
	x[2] = 0.0;
	y[2] = -0.5;
	x[3] = 0.0;
	y[3] = 0.5;
	np = 4;
	break;
    case G_ICON_BOX:
	G_debug(1, "box");
	x[0] = -0.5;
	y[0] = -0.5;
	x[1] = 0.5;
	y[1] = -0.5;
	x[2] = 0.5;
	y[2] = -0.5;
	x[3] = 0.5;
	y[3] = 0.5;
	x[4] = 0.5;
	y[4] = 0.5;
	x[5] = -0.5;
	y[5] = 0.5;
	x[6] = -0.5;
	y[6] = 0.5;
	x[7] = -0.5;
	y[7] = -0.5;
	np = 8;
	break;
    case G_ICON_ARROW:
	x[0] = -1;
	y[0] = 0.5;
	x[1] = 0;
	y[1] = 0.0;
	x[2] = -1;
	y[2] = -0.5;
	x[3] = 0;
	y[3] = 0.0;
	np = 4;
	break;
    }

    trans(x, y, np, angle, scale, xc, yc);

    for (i = 0; i < np; i += 2)
	G_plot_line(x[i], y[i], x[i + 1], y[i + 1]);

    return (1);
}
