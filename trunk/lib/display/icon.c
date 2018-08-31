/*!
  \file lib/display/icon.c
  
  \brief Display Library - Plot icon
  
  (C) 2001-2008, 2012 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author USA-CERL
*/

#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>

static void line(double m[2][3], double x0, double y0, double x1, double y1)
{
	double tx0 = m[0][0] * x0 + m[0][1] * y0 + m[0][2];
	double ty0 = m[1][0] * x0 + m[1][1] * y0 + m[1][2];
	double tx1 = m[0][0] * x1 + m[0][1] * y1 + m[0][2];
	double ty1 = m[1][0] * x1 + m[1][1] * y1 + m[1][2];

	D_line_abs(tx0, ty0, tx1, ty1);
}

/*!
  \brief Plot icon
  
  Supported types:
  - G_ICON_CROSS
  - G_ICON_BOX
  - G_ICON_ARROW
  
  \param xc,yc icon coordinates
  \param type  icon type
  \param angle rotation angle [rad]
  \param scale scale factor
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
    default:
        G_warning(_("Unsupported icon %d"), type);
        break;
    }
}
