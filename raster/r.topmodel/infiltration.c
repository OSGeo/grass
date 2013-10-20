#include <math.h>
#include <grass/raster.h>
#include "global.h"

/* The Green-and-Ampt Model */
double calculate_f(double t, double R)
{
    static double cumf = 0.0, f_ = 0.0;
    static char ponding = 0;
    double f, f1, f2, fc, R2, cnst, pt, psi_dtheta, sum;
    int factorial;
    int i, j;


    /* reset if there is no rainfall */
    if (R <= 0.0) {
	cumf = 0.0;
	f_ = 0.0;
	ponding = 0;
	return 0.0;
    }

    f1 = cnst = pt = 0.0;
    psi_dtheta = params.psi * params.dtheta;
    if (!ponding) {
	if (cumf) {
	    f1 = cumf;
	    R2 = -params.K0 / params.m *
		(psi_dtheta + f1) / (1 - exp(f1 / params.m));
	    /* rainfall intensity is greater than infiltration
	     * rate, so ponding starts */
	    if (R2 < R) {
		f_ = cumf;
		pt = t - input.dt;
		ponding = 1;
		goto cont1;
	    }
	}

	f2 = cumf + R * input.dt;
	R2 = -params.K0 / params.m *
	    (psi_dtheta + f2) / (1 - exp(f2 / params.m));
	/* rainfall intensity is less than infiltration rate. all
	 * rainfall will be infiltrated. */
	if (f2 == 0.0 || R2 > R) {
	    f = R;
	    cumf += f * input.dt;
	    ponding = 0;
	    return f;
	}
	/* rainfall intensity is greater than infiltration rate. */
	f_ = cumf + R2 * input.dt;
	for (i = 0; i < MAXITER; i++) {
	    R2 = -params.K0 / params.m *
		(psi_dtheta + f_) / (1 - exp(f_ / params.m));
	    if (R2 > R) {
		f1 = f_;
		f_ = (f_ + f2) / 2.0;
		f = f_ - f1;
	    }
	    else {
		f2 = f_;
		f_ = (f_ + f1) / 2.0;
		f = f_ - f2;
	    }
	    if (fabs(f) < TOLERANCE)
		break;
	}
	if (i == MAXITER) {
	    Rast_set_d_null_value(&f, 1);
	    return f;
	}
	pt = t - input.dt + (f_ - cumf) / R;
	if (pt > t) {
	    f = R;
	    cumf += f * input.dt;
	    ponding = 0;
	    return f;
	}
      cont1:
	cnst = 0.0;
	factorial = 1;
	fc = (f_ + psi_dtheta);
	for (j = 1; j <= NTERMS; j++) {
	    factorial *= j;
	    cnst += pow(fc / params.m, (double)j) / (double)(j * factorial);
	}
	cnst = log(fc) - (log(fc) + cnst) / exp(psi_dtheta / params.m);
	f_ += R * (t - pt) / 2.0;
	ponding = 1;
    }

    /* Newton-Raphson */
    for (i = 0; i < MAXITER; i++) {
	fc = f_ + psi_dtheta;
	sum = 0.0;
	factorial = 1;
	for (j = 1; j <= NTERMS; j++) {
	    factorial *= j;
	    sum += pow(fc / params.m, (double)j) / (double)(j * factorial);
	}
	f1 = -(log(fc) - (log(fc) + sum) /
	       exp(psi_dtheta / params.m) - cnst) /
	    (params.K0 / params.m) - (t - pt);
	f2 = (exp(f_ / params.m) - 1.0) / (fc * params.K0 / params.m);
	f = -f1 / f2;
	f_ += f;
	if (fabs(f) < TOLERANCE)
	    break;
    }
    if (i == MAXITER) {
	Rast_set_d_null_value(&f, 1);
	return f;
    }

    if (f_ < cumf + R * input.dt) {
	f = (f_ - cumf) / input.dt;
	cumf = f_;
	/* initial guess for next time step */
	f_ += f * input.dt;
    }


    return f;
}
