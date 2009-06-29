#include "Gwater.h"
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

CELL *ril_buf;

int sg_factor(void)
{
    int r, c;
    CELL low_elev, hih_elev;
    double height, length, S, sin_theta;

    G_message(_("SECTION 4: RUSLE LS and/or S factor determination."));

    if (ril_flag)
	ril_buf = Rast_allocate_c_buf();

    for (r = 0; r < nrows; r++) {
	G_percent(r, nrows, 3);
	if (ril_flag) {
	    Rast_get_c_row(ril_fd, ril_buf, r);
	}
	for (c = 0; c < ncols; c++) {
	    low_elev = alt[SEG_INDEX(alt_seg, r, c)];
	    hih_elev = r_h[SEG_INDEX(r_h_seg, r, c)];
	    length = s_l[SEG_INDEX(s_l_seg, r, c)];
	    height = 1.0 * (hih_elev - low_elev) / ele_scale;
	    if (length > max_length) {
		height *= max_length / length;
		length = max_length;
	    }
	    sin_theta = height / sqrt(height * height + length * length);
	    if (height / length < .09)
		S = 10.8 * sin_theta + .03;
	    else
		S = 16.8 * sin_theta - .50;
	    if (sg_flag)
		s_g[SEG_INDEX(s_g_seg, r, c)] = S;
	    if (ls_flag) {
		length *= METER_TO_FOOT;
		len_slp_equ(length, sin_theta, S, r, c);
	    }
	}
    }
    G_percent(nrows, nrows, 1);	/* finish it */

    if (ril_flag) {
	G_free(ril_buf);
	Rast_close(ril_fd);
    }

    return 0;
}

int len_slp_equ(double slope_length, double sin_theta, double S, int r, int c)
{
    double ril, s_l_exp,	/* m                            */
      rill_ratio,		/* Beta                         */
      L;

    rill_ratio = (sin_theta / 0.0896) / (3.0 * pow(sin_theta, 0.8) + 0.56);
    if (ril_flag) {
	ril = ril_buf[c];
    }
    else if (ril_value >= 0.0) {
	ril = ril_value;
    }
    else
	ril = 0.0;
    /* rill_ratio equation from Steve Warren */
    rill_ratio *= .5 + .005 * ril + .0001 * ril * ril;
    s_l_exp = rill_ratio / (1 + rill_ratio);
    L = pow((slope_length / 72.6), s_l_exp);
    l_s[SEG_INDEX(l_s_seg, r, c)] = L * S;

    return 0;
}
