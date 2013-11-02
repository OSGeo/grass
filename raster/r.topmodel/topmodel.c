#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/spawn.h>
#include "global.h"

void create_topidxstats(char *topidx, int ntopidxclasses, char *outtopidxstats)
{
    char input[GPATH_MAX];
    char output[GPATH_MAX];
    char nsteps[32];

    sprintf(input, "input=%s", topidx);
    sprintf(output, "output=%s", outtopidxstats);
    sprintf(nsteps, "nsteps=%d", ntopidxclasses);

    G_message("Creating topographic index statistics file...");
    G_verbose_message("r.stats -Anc %s %s %s ...", input, output, nsteps);

    if (G_spawn("r.stats", "r.stats", "-Anc", input, output, nsteps, NULL) != 0)
	G_fatal_error("r.stats failed");
}

double calculate_lambda(void)
{
    int i;
    double retval;


    retval = 0.0;
    for (i = 1; i < misc.ntopidxclasses; i++)
	retval += topidxstats.Aatb_r[i] *
	    (topidxstats.atb[i] + topidxstats.atb[i - 1]) / 2.0;


    return retval;
}

void initialize(void)
{
    int i, j, t;
    double A1, A2;


    misc.lambda = calculate_lambda();
    misc.lnTe = params.lnTe + log(input.dt);
    misc.vch = params.vch * input.dt;
    misc.vr = params.vr * input.dt;
    misc.qs0 = params.qs0 * input.dt;
    misc.qss = exp(misc.lnTe - misc.lambda);

    misc.tch = (double *)G_malloc(params.nch * sizeof(double));
    misc.tch[0] = params.d[0] / misc.vch;
    for (i = 1; i < params.nch; i++)
	misc.tch[i] = misc.tch[0] + (params.d[i] - params.d[0]) / misc.vr;

    misc.nreaches = (int)misc.tch[params.nch - 1];
    if ((double)misc.nreaches < misc.tch[params.nch - 1])
	misc.nreaches++;
    misc.ndelays = (int)misc.tch[0];

    misc.nreaches -= misc.ndelays;

    misc.Ad = (double *)G_malloc(misc.nreaches * sizeof(double));
    for (i = 0; i < misc.nreaches; i++) {
	t = misc.ndelays + i + 1;
	if (t > misc.tch[params.nch - 1]) {
	    misc.Ad[i] = 1.0;
	}
	else {
	    for (j = 1; j < params.nch; j++) {
		if (t <= misc.tch[j]) {
		    misc.Ad[i] = params.Ad_r[j - 1] +
			(params.Ad_r[j] - params.Ad_r[j - 1]) *
			(t - misc.tch[j - 1]) /
			(misc.tch[j] - misc.tch[j - 1]);
		    break;
		}
	    }
	}
    }


    A1 = misc.Ad[0];
    misc.Ad[0] *= params.A;
    for (i = 1; i < misc.nreaches; i++) {
	A2 = misc.Ad[i];
	misc.Ad[i] = A2 - A1;
	A1 = A2;
	misc.Ad[i] *= params.A;
    }

    misc.Srz = (double **)G_malloc(input.ntimesteps * sizeof(double *));
    misc.Suz = (double **)G_malloc(input.ntimesteps * sizeof(double *));
    for (i = 0; i < input.ntimesteps; i++) {
	misc.Srz[i] = (double *)G_malloc(misc.ntopidxclasses * sizeof(double));
	misc.Suz[i] = (double *)G_malloc(misc.ntopidxclasses * sizeof(double));
    }

    for (i = 0; i < misc.ntopidxclasses; i++) {
	misc.Srz[0][i] = params.Sr0;
	misc.Suz[0][i] = 0.0;
    }

    misc.S_mean = (double *)G_malloc(input.ntimesteps * sizeof(double));
    misc.S_mean[0] = -params.m * log(misc.qs0 / misc.qss);

    misc.Qt = (double *)G_malloc(input.ntimesteps * sizeof(double));
    for (i = 0; i < input.ntimesteps; i++)
	misc.Qt[i] = 0.0;

    for (i = 0; i < misc.ndelays; i++)
	misc.Qt[i] = misc.qs0 * params.A;

    A1 = 0.0;
    for (i = 0; i < misc.nreaches; i++) {
	A1 += misc.Ad[i];
	misc.Qt[misc.ndelays + i] = misc.qs0 * (params.A - A1);
    }


    return;
}

void calculate_flows(void)
{
    int i, j, k;
    double Aatb_r;
    double R;
    double _qo, _qv;


    misc.S = (double **)G_malloc(input.ntimesteps * sizeof(double *));
    misc.Ea = (double **)G_malloc(input.ntimesteps * sizeof(double *));
    misc.ex = (double **)G_malloc(input.ntimesteps * sizeof(double *));

    misc.qt = (double **)G_malloc(input.ntimesteps * sizeof(double *));
    misc.qo = (double **)G_malloc(input.ntimesteps * sizeof(double *));
    misc.qv = (double **)G_malloc(input.ntimesteps * sizeof(double *));

    misc.qs = (double *)G_malloc(input.ntimesteps * sizeof(double));
    misc.f = (double *)G_malloc(input.ntimesteps * sizeof(double));
    misc.fex = (double *)G_malloc(input.ntimesteps * sizeof(double));

    for (i = 0; i < input.ntimesteps; i++) {
	misc.S[i] = (double *)G_malloc(misc.ntopidxclasses * sizeof(double));
	misc.Ea[i] = (double *)G_malloc(misc.ntopidxclasses * sizeof(double));
	misc.ex[i] = (double *)G_malloc(misc.ntopidxclasses * sizeof(double));

	misc.qt[i] = (double *)G_malloc((misc.ntopidxclasses + 1) *
					sizeof(double));
	misc.qo[i] = (double *)G_malloc((misc.ntopidxclasses + 1) *
					sizeof(double));
	misc.qv[i] = (double *)G_malloc((misc.ntopidxclasses + 1) *
					sizeof(double));

	misc.qt[i][misc.ntopidxclasses] = 0.0;
	misc.qo[i][misc.ntopidxclasses] = 0.0;
	misc.qv[i][misc.ntopidxclasses] = 0.0;
	misc.qs[i] = 0.0;

	if (params.infex) {
	    misc.f[i] = input.dt *
		    calculate_infiltration(i + 1, input.R[i] / input.dt);
	    misc.fex[i] = input.R[i] - misc.f[i];
	    R = misc.f[i];
	}
	else {
	    misc.f[i] = 0.0;
	    misc.fex[i] = 0.0;
	    R = input.R[i];
	}

	if (i) {
	    for (j = 0; j < misc.ntopidxclasses; j++) {
		misc.Srz[i][j] = misc.Srz[i - 1][j];
		misc.Suz[i][j] = misc.Suz[i - 1][j];
	    }
	}

	misc.qs[i] = misc.qss * exp(-misc.S_mean[i] / params.m);

	for (j = 0; j < misc.ntopidxclasses; j++) {
	    Aatb_r = (topidxstats.Aatb_r[j] +
		      (j < misc.ntopidxclasses - 1 ? topidxstats.Aatb_r[j + 1]
		       : 0.0)) / 2.0;

	    misc.S[i][j] = misc.S_mean[i] +
		params.m * (misc.lambda - topidxstats.atb[j]);
	    if (misc.S[i][j] < 0.0)
		misc.S[i][j] = 0.0;

	    misc.Srz[i][j] -= R;

	    if (misc.Srz[i][j] < 0.0) {
		misc.Suz[i][j] -= misc.Srz[i][j];
		misc.Srz[i][j] = 0.0;
	    }

	    misc.ex[i][j] = 0.0;
	    if (misc.Suz[i][j] > misc.S[i][j]) {
		misc.ex[i][j] = misc.Suz[i][j] - misc.S[i][j];
		misc.Suz[i][j] = misc.S[i][j];
	    }

	    _qv = 0.0;
	    if (misc.S[i][j] > 0.0) {
		_qv = (params.td > 0.0 ?
		       misc.Suz[i][j] /
		       (misc.S[i][j] * params.td) * input.dt
		       : -params.td * params.K0 *
		       exp(-misc.S[i][j] / params.m));
		if (_qv > misc.Suz[i][j])
		    _qv = misc.Suz[i][j];
		misc.Suz[i][j] -= _qv;
		if (misc.Suz[i][j] < ZERO)
		    misc.Suz[i][j] = 0.0;
		_qv *= Aatb_r;
	    }
	    misc.qv[i][j] = _qv;
	    misc.qv[i][misc.ntopidxclasses] += misc.qv[i][j];

	    misc.Ea[i][j] = 0.0;
	    if (input.Ep[i] > 0.0) {
		misc.Ea[i][j] = input.Ep[i] *
		    (1 - misc.Srz[i][j] / params.Srmax);
		if (misc.Ea[i][j] > params.Srmax - misc.Srz[i][j])
		    misc.Ea[i][j] = params.Srmax - misc.Srz[i][j];
	    }
	    misc.Srz[i][j] += misc.Ea[i][j];

	    _qo = 0.0;
	    if (j > 0) {
		if (misc.ex[i][j] > 0.0)
		    _qo = topidxstats.Aatb_r[j] *
			(misc.ex[i][j - 1] + misc.ex[i][j]) / 2.0;
		else if (misc.ex[i][j - 1] > 0.0)
		    _qo = Aatb_r * misc.ex[i][j - 1] /
			(misc.ex[i][j - 1] -
			 misc.ex[i][j]) * misc.ex[i][j - 1] / 2.0;
	    }
	    misc.qo[i][j] = _qo;
	    misc.qo[i][misc.ntopidxclasses] += misc.qo[i][j];

	    misc.qt[i][j] = misc.qo[i][j] + misc.qs[i];
	}
	misc.qo[i][misc.ntopidxclasses] += misc.fex[i];
	misc.qt[i][misc.ntopidxclasses] = misc.qo[i][misc.ntopidxclasses] + misc.qs[i];

	misc.S_mean[i] = misc.S_mean[i] +
	    misc.qs[i] - misc.qv[i][misc.ntopidxclasses];

	if (i + 1 < input.ntimesteps)
	    misc.S_mean[i + 1] = misc.S_mean[i];

	for (j = 0; j < misc.nreaches; j++) {
	    k = i + j + misc.ndelays;
	    if (k > input.ntimesteps - 1)
		break;
	    misc.Qt[k] += misc.qt[i][misc.ntopidxclasses] * misc.Ad[j];
	}
    }


    return;
}

/* Objective function for hydrograph suggested by Servet and Dezetter(1991) */
double calculate_efficiency(void)
{
    int i;
    double Em, numerator, denominator;


    misc.Qobs_mean = 0.0;
    numerator = 0.0;
    for (i = 0; i < input.ntimesteps; i++) {
	misc.Qobs_mean += misc.Qobs[i];
	numerator += pow(misc.Qobs[i] - misc.Qt[i], 2.0);
    }
    misc.Qobs_mean /= input.ntimesteps;

    denominator = 0.0;
    for (i = 0; i < input.ntimesteps; i++)
	denominator += pow(misc.Qobs[i] - misc.Qobs_mean, 2.0);

    if (denominator == 0.0) {
	G_warning("Em can not be resolved due to constant observed Q");
	Rast_set_d_null_value(&Em, 1);
    }
    else {
	Em = 1.0 - numerator / denominator;
    }

    return Em;
}

void calculate_others(void)
{
    int i;


    misc.Qt_mean = 0.0;
    for (i = 0; i < input.ntimesteps; i++) {
	misc.Qt_mean += misc.Qt[i];
	if (!i || misc.Qt_peak < misc.Qt[i]) {
	    misc.Qt_peak = misc.Qt[i];
	    misc.tt_peak = i + 1;
	}
    }
    misc.Qt_mean /= input.ntimesteps;

    if (file.qobs) {
	misc.Em = calculate_efficiency();
	for (i = 0; i < input.ntimesteps; i++) {
	    if (!i || misc.Qobs_peak < misc.Qobs[i]) {
		misc.Qobs_peak = misc.Qobs[i];
		misc.tobs_peak = i + 1;
	    }
	}
    }

    return;
}

void run_topmodel(void)
{
    initialize();
    calculate_flows();
    calculate_others();

    return;
}
