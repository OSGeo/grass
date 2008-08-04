#include <grass/gis.h>
#include "global.h"


double get_lambda(void)
{
    int i;
    double retval;


    retval = 0.0;
    for (i = 1; i < misc.nidxclass; i++)
	retval += idxstats.Aatb_r[i] *
	    (idxstats.atb[i] + idxstats.atb[i - 1]) / 2.0;


    return retval;
}


void initialize(void)
{
    int i, j, t;
    double A1, A2;


    misc.lambda = get_lambda();
    misc.lnTe = params.lnTe + log(input.dt);
    misc.vch = params.vch * input.dt;
    misc.vr = params.vr * input.dt;
    misc.qs0 = params.qs0 * input.dt;
    misc.qss = exp(misc.lnTe - misc.lambda);

    misc.tch = (double *)G_malloc(params.nch * sizeof(double));
    misc.tch[0] = params.d[0] / misc.vch;
    for (i = 1; i < params.nch; i++)
	misc.tch[i] = misc.tch[0] + (params.d[i] - params.d[0]) / misc.vr;

    misc.nreach = (int)misc.tch[params.nch - 1];
    if ((double)misc.nreach < misc.tch[params.nch - 1])
	misc.nreach++;
    misc.ndelay = (int)misc.tch[0];

    misc.nreach -= misc.ndelay;

    misc.Ad = (double *)G_malloc(misc.nreach * sizeof(double));
    for (i = 0; i < misc.nreach; i++) {
	t = misc.ndelay + i + 1;
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
    for (i = 1; i < misc.nreach; i++) {
	A2 = misc.Ad[i];
	misc.Ad[i] = A2 - A1;
	A1 = A2;
	misc.Ad[i] *= params.A;
    }

    misc.Srz = (double **)G_malloc(input.ntimestep * sizeof(double *));
    misc.Suz = (double **)G_malloc(input.ntimestep * sizeof(double *));
    for (i = 0; i < input.ntimestep; i++) {
	misc.Srz[i] = (double *)G_malloc(misc.nidxclass * sizeof(double));
	misc.Suz[i] = (double *)G_malloc(misc.nidxclass * sizeof(double));
    }

    for (i = 0; i < misc.nidxclass; i++) {
	misc.Srz[0][i] = params.Sr0;
	misc.Suz[0][i] = 0.0;
    }

    misc.S_mean = (double *)G_malloc(input.ntimestep * sizeof(double));
    misc.S_mean[0] = -params.m * log(misc.qs0 / misc.qss);

    misc.Qt = (double *)G_malloc(input.ntimestep * sizeof(double));
    for (i = 0; i < input.ntimestep; i++)
	misc.Qt[i] = 0.0;

    for (i = 0; i < misc.ndelay; i++)
	misc.Qt[i] = misc.qs0 * params.A;

    A1 = 0.0;
    for (i = 0; i < misc.nreach; i++) {
	A1 += misc.Ad[i];
	misc.Qt[misc.ndelay + i] = misc.qs0 * (params.A - A1);
    }


    return;
}


void implement(void)
{
    int i, j, k;
    double Aatb_r;
    double R;
    double _qo, _qv;


    misc.S = (double **)G_malloc(input.ntimestep * sizeof(double *));
    misc.Ea = (double **)G_malloc(input.ntimestep * sizeof(double *));
    misc.ex = (double **)G_malloc(input.ntimestep * sizeof(double *));

    misc.qt = (double **)G_malloc(input.ntimestep * sizeof(double *));
    misc.qo = (double **)G_malloc(input.ntimestep * sizeof(double *));
    misc.qv = (double **)G_malloc(input.ntimestep * sizeof(double *));

    misc.qs = (double *)G_malloc(input.ntimestep * sizeof(double));
    misc.f = (double *)G_malloc(input.ntimestep * sizeof(double));
    misc.fex = (double *)G_malloc(input.ntimestep * sizeof(double));

    for (i = 0; i < input.ntimestep; i++) {
	misc.S[i] = (double *)G_malloc(misc.nidxclass * sizeof(double));
	misc.Ea[i] = (double *)G_malloc(misc.nidxclass * sizeof(double));
	misc.ex[i] = (double *)G_malloc(misc.nidxclass * sizeof(double));

	misc.qt[i] = (double *)G_malloc((misc.nidxclass + 1) *
					sizeof(double));
	misc.qo[i] = (double *)G_malloc((misc.nidxclass + 1) *
					sizeof(double));
	misc.qv[i] = (double *)G_malloc((misc.nidxclass + 1) *
					sizeof(double));

	misc.qt[i][misc.nidxclass] = 0.0;
	misc.qo[i][misc.nidxclass] = 0.0;
	misc.qv[i][misc.nidxclass] = 0.0;
	misc.qs[i] = 0.0;

	if (params.infex) {
	    misc.f[i] = input.dt *
		get_f((i + 1) * input.dt, input.R[i] / input.dt);
	    misc.fex[i] = input.R[i] - misc.f[i];
	    R = misc.f[i];
	}
	else {
	    misc.f[i] = 0.0;
	    misc.fex[i] = 0.0;
	    R = input.R[i];
	}

	if (i) {
	    for (j = 0; j < misc.nidxclass; j++) {
		misc.Srz[i][j] = misc.Srz[i - 1][j];
		misc.Suz[i][j] = misc.Suz[i - 1][j];
	    }
	}

	misc.qs[i] = misc.qss * exp(-misc.S_mean[i] / params.m);

	for (j = 0; j < misc.nidxclass; j++) {
	    Aatb_r = (idxstats.Aatb_r[j] +
		      (j < misc.nidxclass - 1 ? idxstats.Aatb_r[j + 1]
		       : 0.0)) / 2.0;

	    misc.S[i][j] = misc.S_mean[i] +
		params.m * (misc.lambda - idxstats.atb[j]);
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
	    misc.qv[i][misc.nidxclass] += misc.qv[i][j];

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
		    _qo = idxstats.Aatb_r[j] *
			(misc.ex[i][j - 1] + misc.ex[i][j]) / 2.0;
		else if (misc.ex[i][j - 1] > 0.0)
		    _qo = Aatb_r * misc.ex[i][j - 1] /
			(misc.ex[i][j - 1] -
			 misc.ex[i][j]) * misc.ex[i][j - 1] / 2.0;
	    }
	    misc.qo[i][j] = _qo;
	    misc.qo[i][misc.nidxclass] += misc.qo[i][j];

	    misc.qt[i][j] = misc.qo[i][j] + misc.qs[i];
	}
	misc.qo[i][misc.nidxclass] += misc.fex[i];
	misc.qt[i][misc.nidxclass] = misc.qo[i][misc.nidxclass] + misc.qs[i];

	misc.S_mean[i] = misc.S_mean[i] +
	    misc.qs[i] - misc.qv[i][misc.nidxclass];

	if (i + 1 < input.ntimestep)
	    misc.S_mean[i + 1] = misc.S_mean[i];

	for (j = 0; j < misc.nreach; j++) {
	    k = i + j + misc.ndelay;
	    if (k > input.ntimestep - 1)
		break;
	    misc.Qt[k] += misc.qt[i][misc.nidxclass] * misc.Ad[j];
	}
    }


    return;
}


/* Object function for hydrograph suggested by Servet and Dezetter(1991) */
double get_Em(void)
{
    int i;
    double Em, numerator, denominator;


    misc.Qobs_mean = 0.0;
    numerator = 0.0;
    for (i = 0; i < input.ntimestep; i++) {
	misc.Qobs_mean += misc.Qobs[i];
	numerator += pow(misc.Qobs[i] - misc.Qt[i], 2.0);
    }
    misc.Qobs_mean /= input.ntimestep;

    denominator = 0.0;
    for (i = 0; i < input.ntimestep; i++)
	denominator += pow(misc.Qobs[i] - misc.Qobs_mean, 2.0);

    if (denominator == 0.0) {
	G_warning("Em can not be resolved due to constant " "observed Q");
	G_set_d_null_value(&Em, 1);
    }
    else {
	Em = 1.0 - numerator / denominator;
    }


    return Em;
}


void others(void)
{
    int i;


    misc.Qt_mean = 0.0;
    for (i = 0; i < input.ntimestep; i++) {
	misc.Qt_mean += misc.Qt[i];
	if (!i || misc.Qt_peak < misc.Qt[i]) {
	    misc.Qt_peak = misc.Qt[i];
	    misc.tt_peak = i + 1;
	}
    }
    misc.Qt_mean /= input.ntimestep;

    if (file.Qobs) {
	misc.Em = get_Em();
	for (i = 0; i < input.ntimestep; i++) {
	    if (!i || misc.Qobs_peak < misc.Qobs[i]) {
		misc.Qobs_peak = misc.Qobs[i];
		misc.tobs_peak = i + 1;
	    }
	}
    }


    return;
}


void topmodel(void)
{
    initialize();
    implement();
    others();


    return;
}
