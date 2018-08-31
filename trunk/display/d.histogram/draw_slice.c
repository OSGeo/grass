#include <math.h>
#include <grass/display.h>
#include <grass/gis.h>

char percent[] = "%";

int draw_slice(struct Colors *colors, int fill_flag, DCELL fill_color1, DCELL fill_color2, int txt_color, double cx, double cy, double r,	/* in normalized coords. */
	       double a1, double a2	/* in degrees */
    )
{
    double tt, tb, tr, tl;
    int height, width;
    double yoffset, xoffset;
    double x[1000], y[1000];
    int lx, ly;
    int i = 1;
    char txt[512];
    double arc, arc_incr = 0.01;
    DCELL fill_color;

    D_get_src(&tt, &tb, &tl, &tr);

    height = tb - tt;
    width = tr - tl;
    yoffset = tb;
    xoffset = tl;

    while (a2 / arc_incr > 998)
	arc_incr *= 2;

    x[0] = (xoffset + cx * width);
    y[0] = (yoffset - cy * height);

    arc = a1;
    if (fill_flag && fill_color1 != fill_color2) {
	i = 1;
	while (arc <= (a1 + a2)) {
	    fill_color = fill_color1 + (arc - a1) *
		(fill_color2 - fill_color1) / a2;
	    x[i] = x[0] + r * (width) * cos(arc / 57.296);
	    y[i] = y[0] - r * (height) * sin(arc / 57.296);
	    if (i == 2) {
		D_d_color(fill_color, colors);
		D_polygon_abs(x + i - 2, y + i - 2, 3);
		x[i - 1] = x[i];
		y[i - 1] = y[i];
	    }
	    else
		i++;
	    arc = arc + arc_incr;
	}
    }
    else {
	i = 1;
	while (arc <= (a1 + a2)) {
	    x[i] = x[0] + r * (width) * cos(arc / 57.296);
	    y[i] = y[0] - r * (height) * sin(arc / 57.296);
	    i++;
	    arc = arc + arc_incr;
	}

	if (!fill_flag) {
	    D_use_color(txt_color);
	    D_polyline_abs(x, y, i);
	}
	else {
	    D_d_color(fill_color1, colors);
	    D_polygon_abs(x, y, i);
	}
    }

    if (a2 > 15.0) {
	/* draw a label */
	arc = a1 + a2 / 2;
	sprintf(txt, "%2.0f%s", (a2 / 360.0) * 100.0, percent);
	D_get_text_box(txt, &tt, &tb, &tl, &tr);
	lx = x[0] + (r + 0.03) * (width) * cos(arc / 57.296) - (tr - tl) / 2;
	ly = y[0] - (r + 0.03) * (height) * sin(arc / 57.296) + (tb - tt) / 2;
	D_pos_abs(lx, ly);
	D_use_color(txt_color);
	D_text(txt);
    }

    return 0;
}

int draw_slice_unfilled(struct Colors *colors, int tc, double cx, double cy,	/* circle center, in normalized coords. */
			double r,	/* circle radius, in normalized units */
			double a1,	/* arc start position, in degrees */
			double a2	/* arc size, in degrees */
    )
{
    draw_slice(colors, 0, 0., 0., tc, cx, cy, r, a1, a2);

    return 0;
}


int draw_slice_filled(struct Colors *colors, DCELL fc,	/* fill color */
		      int tc,	/* text color */
		      double cx, double cy,	/* circle center, in normalized coords. */
		      double r,	/* circle radius, in normalized units */
		      double a1,	/* arc start position, in degrees */
		      double a2	/* arc size, in degrees */
    )
{
    draw_slice(colors, 1, fc, fc, tc, cx, cy, r, a1, a2);

    return 0;
}
