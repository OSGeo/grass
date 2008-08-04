#include "Gwater.h"
#include <grass/gis.h>
#include <grass/glocale.h>


int sg_factor(void)
{
    int r, c;
    CELL downer, low_elev, hih_elev;
    double height, length, S, sin_theta;

    G_message(_("SECTION 4: Length Slope determination."));
    for (r = nrows - 1; r >= 0; r--) {
	G_percent(r, nrows, 3);
	for (c = ncols - 1; c >= 0; c--) {
	    cseg_get(&alt, &low_elev, r, c);
	    cseg_get(&r_h, &hih_elev, r, c);
	    dseg_get(&s_l, &length, r, c);
	    cseg_get(&asp, &downer, r, c);
	    height = hih_elev - low_elev;
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
		S *= 100.0;
		dseg_put(&s_g, &S, r, c);
	    }
	}
    }
    G_percent(r, nrows, 3);	/* finish it */

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
    LS = S * 100 * pow((slope_length / 72.6), s_l_exp);
    dseg_put(&l_s, &LS, r, c);

    return 0;
}
