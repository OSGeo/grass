
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
#include <grass/display.h>

static void line(double m[2][3], double x0, double y0, double x1, double y1)
{
	double tx0 = m[0][0] * x0 + m[0][1] * y0 + m[0][2];
	double ty0 = m[1][0] * x0 + m[1][1] * y0 + m[1][2];
	double tx1 = m[0][0] * x1 + m[0][1] * y1 + m[0][2];
	double ty1 = m[1][0] * x1 + m[1][1] * y1 + m[1][2];

	D_line(tx0, ty0, tx1, ty1);
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
void D_plot_icon(double xc, double yc, int type, double angle, double scale)
{
    static double old_a = 1e299, old_s = 0;
    static double sin_a, cos_a;
    static double m[2][3];

    G_debug(2, "D_plot_icon(): xc=%g, yc=%g", xc, yc);

    if (angle != old_a) {
	sin_a = sin(angle);
	cos_a = cos(angle);
    }
    if (angle != old_a || scale != old_s) {
	m[0][0] = cos_a * scale;
	m[0][1] = -sin_a * scale;
	m[1][0] = sin_a * scale;
	m[1][1] = cos_a * scale;
    }
    m[0][2] = xc;
    m[1][2] = yc;

    switch (type) {
    case G_ICON_CROSS:
	line(m, -0.5, 0.0, 0.5, 0.0);
	line(m, 0.0, -0.5, 0.0, 0.5);
	break;
    case G_ICON_BOX:
	line(m, -0.5, -0.5, 0.5, -0.5);
	line(m, 0.5, -0.5, 0.5, 0.5);
	line(m, 0.5, 0.5, -0.5, 0.5);
	line(m, -0.5, 0.5, -0.5, -0.5);
	break;
    case G_ICON_ARROW:
	line(m, -1, 0.5, 0, 0.0);
	line(m, -1, -0.5, 0, 0.0);
	break;
    }
}
