#include "Gwater.h"
#include <grass/gis.h>
#include <grass/glocale.h>


int sg_factor(void)
{
    int r, c;
    CELL low_elev, hih_elev;
    double height, length, S, sin_theta;
    WAT_ALT wa;
    ASP_FLAG af;

    G_message(_("SECTION 5: RUSLE LS and/or S factor determination."));
    for (r = nrows - 1; r >= 0; r--) {
	G_percent(nrows - r, nrows, 3);
	for (c = ncols - 1; c >= 0; c--) {
	    seg_get(&aspflag, (char *)&af, r, c);
	    if (FLAG_GET(af.flag, NULLFLAG))
		continue;

	    seg_get(&watalt, (char *)&wa, r, c);
	    low_elev = wa.ele;
	    cseg_get(&r_h, &hih_elev, r, c);
	    dseg_get(&s_l, &length, r, c);
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
	    if (ls_flag) {
		length *= METER_TO_FOOT;
		len_slp_equ(length, sin_theta, S, r, c);
	    }
	    if (sg_flag) {
		dseg_put(&s_g, &S, r, c);
	    }
	}
    }
    G_percent(nrows, nrows, 1);	/* finish it */

    return 0;
}

int len_slp_equ(double slope_length, double sin_theta, double S, int r, int c)
{
    double rill, s_l_exp,	/* m                            */
      rill_ratio,		/* Beta                         */
      LS;

    rill_ratio = (sin_theta / 0.0896) / (3.0 * pow(sin_theta, 0.8) + 0.56);
    if (ril_flag) {
	dseg_get(&ril, &rill, r, c);
    }
    else if (ril_value >= 0.0) {
	rill = ril_value;
    }
    else
	rill = 0.0;
    /* rill_ratio equation from Steve Warren */
    rill_ratio *= .5 + .005 * rill + .0001 * rill * rill;
    s_l_exp = rill_ratio / (1 + rill_ratio);
    LS = S * pow((slope_length / 72.6), s_l_exp);
    dseg_put(&l_s, &LS, r, c);

    return 0;
}
