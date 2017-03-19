#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "global.h"

/* Calculates the infiltrate rate (m/hr) for the given timestep. For variable
 * names and equation numbers in comments, refer to Beven (1984).
 *
 * Beven, K. J., 1984. Infiltration into a class of vertically non-uniform
 * soils. Hydrological Sciences Journal 29 (4), 425-434.
 *
 * Beven, K. J., Kirkby, M. J., 1979. A physically based, variable contributing
 * area model of basin hydrology. Hydrological Sciences Bulletin 24 (1), 43-69.
 *
 * Morel-Seytoux, H. J., Khanji, J., 1974. Derivation of an equation of
 * infiltration. Water Resources Research 10 (4), 795-800.
 */
double calculate_infiltration(int timestep, double R)
{
    /* params.K0	Surface hydraulic conductivity (m/h)
     * params.psi	Wetting front suction (m)
     * params.dtheta	Water content change across the wetting front
     *			    dtheta = saturated moisture content
     *				     - initial moisture content
     * params.m		Parameter controlling the decline rate of
     *			transmissivity (m)
     *
     *			Beven and Kirkby (1979) introduced the scaling
     *			parameter m.
     *
     *			    K(z) = K0 * exp(-f * z)
     *
     *			where K(z) is hydraulic conductivity at depth z,
     *			      z is the soil depth, and
     *			      f is the parameter controlling the decline rate
     *			        of transmissivity (1/m); can be defined by m as
     *			        f = dtheta / m
     *
     *			Now, m = dtheta / f.
     *
     * R		Rainfall intensity (m/h)
     * r		Infiltration rate (m/h)
     * cumI		Cumulative infiltration at the start of timestep (m)
     * I		Cumulative infiltration at the end of timestep (m)
     * dIdt		Infiltration rate for the current timestep (m/hr)
     * C		Storage-suction factor (m) (Morel-Seytoux and Khanji,
     *			1974); C = psi * dtheta
     * IC		I + C
     * lambda		lambda in Eq. (8); Note that this lambda is different
     *			from params.lambda
     * t		Current time (hr)
     * tp		Time to ponding (hr)
     */
    static double cumI = 0.0, I = 0.0;
    static char ponding = 0;
    double r, C, IC, lambda, dIdt, t, tp;
    double f1, f2, df, sum;
    int fact;
    int i, j;

    /* reset if there is no rainfall */
    if (R <= 0.0) {
	cumI = 0.0;
	I = 0.0;
	ponding = 0;
	return 0.0;
    }

    t = timestep * input.dt;
    lambda = tp = f1 = 0.0;
    C = params.psi * params.dtheta;

    /* if no ponding occurred yet */
    if (!ponding) {
	if (cumI > 0) {
	    f1 = cumI;
	    /* infiltration rate: Eq. (6)
	     * Note that his Ks = K0 * exp(f * z) in Eq. (1a) instead of
	     * Ks = K0 * exp(-f * z) used in his TOPMODEL code, TMOD9502.F.
	     * Substitute f=-dtheta/m for f in Eq. (1a), hence -K0 and
	     * exp(f1/m), slightly different from the original Eq. (6).
	     */
	    r = -params.K0 / params.m * (C + f1) / (1 - exp(f1 / params.m));
	    /* rainfall intensity is greater than infiltration
	     * rate, so ponding starts */
	    if (r < R) {
		I = cumI;
		tp = t - input.dt;
		ponding = 1;
		goto cont1;
	    }
	}

	/* try full infiltration */
	f2 = cumI + R * input.dt;
	/* infiltration rate */
	r = -params.K0 / params.m * (C + f2) / (1 - exp(f2 / params.m));
	/* if infiltration rate is greater than rainfall intensity, all
	 * rainfall will be infiltrated */
	if (f2 == 0.0 || r > R)
	    goto cont2;

	/* infiltration rate is less than rainfall intensity */
	/* Newton-Raphson iteration to solve Eq. (6) for I */
	/* guess new cumulative infiltration */
	I = cumI + r * input.dt;
	for (i = 0; i < MAXITER; i++) {
	    /* new infiltration rate */
	    r = -params.K0 / params.m * (C + I) / (1 - exp(I / params.m));
	    /* if new infiltration rate is greater than rainfall intensity,
	     * increase cumulative infiltration
	     */
	    if (r > R) {
		f1 = I;
		I = (I + f2) / 2.0;
		df = I - f1;
	    }
	    /* otherwise, decrease cumulative infiltration */
	    else {
		f2 = I;
		I = (I + f1) / 2.0;
		df = I - f2;
	    }
	    /* stop if cumulative infiltration converged */
	    if (fabs(df) <= TOLERANCE)
		break;
	}
	if (i == MAXITER)
	    G_warning(
		_("Maximum number of iterations exceeded at timestep %d"),
		timestep);

	/* ponding time */
	tp = t - input.dt + (I - cumI) / R;
	/* if ponding time is greater than the current time,
	 * tp = t - dt + (I - cumI) / R > t
	 * (I - cumI) / R > dt
	 * I - cumI > R * dt
	 * means that additional infiltration (I - cumI) is greater than the
	 * total rainfall (R * dt), which cannot happen when there is no
	 * ponding, so infiltrate all rainfall
	 */
	if (tp > t)
	    goto cont2;

      cont1:
	/* if additional infiltration is less than the total rainfall, ponding
	 * starts
	 */
	lambda = 0.0;
	fact = 1;
	IC = I + C;
	for (j = 1; j <= NTERMS; j++) {
	    fact *= j;
	    lambda += pow(IC / params.m, (double)j) / (double)(j * fact);
	}
	/* lambda in Eq. (8) */
	lambda = log(IC) - (log(IC) + lambda) / exp(C / params.m);
	I += R * (t - tp) / 2.0;
	ponding = 1;
    }

    /* Newton-Raphson iteration to solve Eq. (8) for I */
    for (i = 0; i < MAXITER; i++) {
	IC = I + C;
	sum = 0.0;
	fact = 1;
	for (j = 1; j <= NTERMS; j++) {
	    fact *= j;
	    sum += pow(IC / params.m, (double)j) / (double)(j * fact);
	}
	/* Eq. (8) - (t - tp) in hr: should converge to 0
	 * Note that sum is outside 1/exp(C/m) in Eq. (8), but inside in his
	 * TMOD9502.F. Based on lambda and his code, it looks like a typo in
	 * Eq. (8).
	 */
	f1 = -(log(IC) - (log(IC) + sum) / exp(C / params.m) - lambda) /
	    (params.K0 / params.m) - (t - tp);
	/* inverse of Eq. (7) in hr/m */
	f2 = (exp(I / params.m) - 1.0) / (IC * params.K0 / params.m);
	/* -(Eq. (8) - (t-tp)) * Eq. (7): cumulative infiltration in a short
	 * time period */
	df = -f1 / f2;
	I += df;
	if (fabs(df) <= TOLERANCE)
	    break;
    }
    if (i == MAXITER)
	G_warning(_("Maximum number of iterations exceeded at timestep %d"),
			timestep);

    /* if new cumulative infiltration is less than the previous cumulative
     * infiltration plus the total rainfall, update the current cumulative
     * infiltration and guess cumulative infiltration for the next time step
     */
    if (I < cumI + R * input.dt) {
	dIdt = (I - cumI) / input.dt;
	cumI = I;
	/* initial guess for next time step */
	I += dIdt * input.dt;
	return dIdt;
    }

cont2:
    /* infiltrate all rainfall */
    dIdt = R;
    cumI += dIdt * input.dt;
    ponding = 0;

    return dIdt;
}
