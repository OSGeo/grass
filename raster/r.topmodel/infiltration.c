#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "global.h"

/* The Green-and-Ampt Model */
double calculate_infiltration(int timestep, double R)
{
    static double cumf = 0.0, f = 0.0;
    static char ponding = 0;
    double t, df, f1, f2, fc, R2, cnst, pt, psi_dtheta, sum;
    int factorial;
    int i, j;

    t = timestep * input.dt;
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
		f = cumf;
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
	if (f2 == 0.0 || R2 > R)
	    goto cont2;

	/* rainfall intensity is greater than infiltration rate. */
	f = cumf + R2 * input.dt;
	for (i = 0; i < MAXITER; i++) {
	    R2 = -params.K0 / params.m *
		(psi_dtheta + f) / (1 - exp(f / params.m));
	    if (R2 > R) {
		f1 = f;
		f = (f + f2) / 2.0;
		df = f - f1;
	    }
	    else {
		f2 = f;
		f = (f + f1) / 2.0;
		df = f - f2;
	    }
	    if (fabs(df) <= TOLERANCE)
		break;
	}
	if (i == MAXITER)
	    G_warning(
		_("Maximum number of iterations exceeded at timestep %d."),
		timestep);

	pt = t - input.dt + (f - cumf) / R;
	if (pt > t)
	    goto cont2;

      cont1:
	cnst = 0.0;
	factorial = 1;
	fc = f + psi_dtheta;
	for (j = 1; j <= NTERMS; j++) {
	    factorial *= j;
	    cnst += pow(fc / params.m, (double)j) / (double)(j * factorial);
	}
	cnst = log(fc) - (log(fc) + cnst) / exp(psi_dtheta / params.m);
	f += R * (t - pt) / 2.0;
	ponding = 1;
    }

    /* Newton-Raphson */
    for (i = 0; i < MAXITER; i++) {
	fc = f + psi_dtheta;
	sum = 0.0;
	factorial = 1;
	for (j = 1; j <= NTERMS; j++) {
	    factorial *= j;
	    sum += pow(fc / params.m, (double)j) / (double)(j * factorial);
	}
	f1 = -(log(fc) - (log(fc) + sum) /
	       exp(psi_dtheta / params.m) - cnst) /
	    (params.K0 / params.m) - (t - pt);
	f2 = (exp(f / params.m) - 1.0) / (fc * params.K0 / params.m);
	df = -f1 / f2;
	f += df;
	if (fabs(df) <= TOLERANCE)
	    break;
    }
    if (i == MAXITER)
	G_warning(_("Maximum number of iterations exceeded at timestep %d."),
			timestep);

    if (f < cumf + R * input.dt) {
	df = (f - cumf) / input.dt;
	cumf = f;
	/* initial guess for next time step */
	f += df * input.dt;
	return df;
    }

cont2:
    df = R;
    cumf += df * input.dt;
    ponding = 0;

    return df;
}
