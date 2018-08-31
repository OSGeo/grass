#include <math.h>
#include <grass/raster.h>
#include <grass/spawn.h>
#include <grass/glocale.h>
#include "global.h"

#define	ZERO	0.0000001

void create_topidxstats(char *topidx, int ntopidxclasses, char *outtopidxstats)
{
    char input[GPATH_MAX], nsteps[32];
    const char *args[5];
    struct Popen child;
    FILE *fp;
    double *atb, *Aatb_r, delta, prev_atb2;
    int i;
    int total_ncells;

    sprintf(input, "input=%s", topidx);
    sprintf(nsteps, "nsteps=%d", ntopidxclasses - 1);

    G_message("Creating topographic index statistics file...");
    G_verbose_message("r.stats -nc %s %s ...", input, nsteps);

    args[0] = "r.stats";
    args[1] = "-nc";
    args[2] = input;
    args[3] = nsteps;
    args[4] = NULL;

    if ((fp = G_popen_read(&child, "r.stats", args)) == NULL)
	G_fatal_error(_("Unable to run %s"), "r.stats");

    atb = (double *)G_malloc(ntopidxclasses * sizeof(double));
    Aatb_r = (double *)G_malloc(ntopidxclasses * sizeof(double));

    total_ncells = 0;
    delta = -1.0;
    prev_atb2 = 0.0;

    for (i = 0; i < ntopidxclasses - 1 && !feof(fp);) {
	double atb1, atb2;
	int ncells;

	get_line(fp, buf);
	if (sscanf(buf, "%lf-%lf %d", &atb1, &atb2, &ncells) == 3) {
	    if (delta < 0)
		delta = atb2 - atb1;
	    else if (atb1 > prev_atb2 + 0.5 * delta) {
		/* r.stats doesn't report non-existing ranges at all. Use 0.5 *
		 * delta to avoid comparing two almost same double numbers. */
		while (prev_atb2 < atb1 - 0.5 * delta) {
		    atb[i] = prev_atb2;
		    Aatb_r[i++] = 0.0;
		    prev_atb2 += delta;
		}
	    }

	    atb[i] = atb1;
	    Aatb_r[i] = (double)ncells;
	    total_ncells += ncells;
	    prev_atb2 = atb2;

	    if (++i == ntopidxclasses - 1) {
		atb[i] = atb2;
		Aatb_r[i] = 0.0;
	    }
	}
    }

    G_popen_close(&child);

    if (i < ntopidxclasses - 1)
	G_fatal_error(_("Invalid %s output"), "r.stats");

    if ((fp = fopen(outtopidxstats, "w")) == NULL)
	G_fatal_error(_("Unable to create output file <%s>"), outtopidxstats);

    for (i = ntopidxclasses - 1; i >= 0; i--)
	fprintf(fp, "%10.3e %10.3e\n", atb[i], Aatb_r[i] / total_ncells);

    fclose(fp);
}

/* Calculate the areal average of topographic index */
double calculate_lambda(void)
{
    int i;
    double lambda;

    lambda = 0.0;
    for (i = 1; i < misc.ntopidxclasses; i++)
	lambda += topidxstats.Aatb_r[i] *
	    (topidxstats.atb[i] + topidxstats.atb[i - 1]) / 2.0;

    return lambda;
}

/* Initialize the flows */
void initialize(void)
{
    int i, j;
    double A1, A2;

    /* average topographic index */
    misc.lambda = calculate_lambda();

    /* ln of the average transmissivity at the soil surface */
    misc.lnTe = params.lnTe + log(input.dt);

    /* main channel routing velocity */
    misc.vch = params.vch * input.dt;

    /* internal subcatchment routing velocity */
    misc.vr = params.vr * input.dt;

    /* initial subsurface flow per unit area */
    misc.qs0 = params.qs0 * input.dt;

    /* saturated subsurface flow per unit area */
    misc.qss = exp(misc.lnTe - misc.lambda);

    misc.tch = (double *)G_malloc(params.nch * sizeof(double));

    /* routing time in the main channel */
    misc.tch[0] = params.d[0] / misc.vch;
    for (i = 1; i < params.nch; i++)
	/* routing time in each internal subcatchment channel */
	misc.tch[i] = misc.tch[0] + (params.d[i] - params.d[0]) / misc.vr;

    /* time of concentration */
    misc.tc = (int)misc.tch[params.nch - 1];
    if ((double)misc.tc < misc.tch[params.nch - 1])
	misc.tc++;

    /* routing delay in the main channel */
    misc.delay = (int)misc.tch[0];

    /* time of concentration in the subcatchment */
    misc.tcsub = misc.tc - misc.delay;

    /* cumulative ratio of the contribution area for each time step */
    misc.Ad = (double *)G_malloc(misc.tcsub * sizeof(double));
    for (i = 0; i < misc.tcsub; i++) {
	int t;

	t = misc.delay + i + 1;
	if (t > misc.tch[params.nch - 1])
	    misc.Ad[i] = params.A;
	else {
	    for (j = 1; j < params.nch; j++) {
		if (t <= misc.tch[j]) {
		    misc.Ad[i] = params.Ad[j - 1] +
			(params.Ad[j] - params.Ad[j - 1]) *
			(t - misc.tch[j - 1]) /
			(misc.tch[j] - misc.tch[j - 1]);
		    break;
		}
	    }
	}
    }

    /* difference in the contribution area for each time step */
    A1 = misc.Ad[0];
    for (i = 1; i < misc.tcsub; i++) {
	A2 = misc.Ad[i];
	misc.Ad[i] = A2 - A1;
	A1 = A2;
    }

    misc.Srz = (double **)G_malloc(input.ntimesteps * sizeof(double *));
    misc.Suz = (double **)G_malloc(input.ntimesteps * sizeof(double *));
    for (i = 0; i < input.ntimesteps; i++) {
	misc.Srz[i] = (double *)G_malloc(misc.ntopidxclasses * sizeof(double));
	misc.Suz[i] = (double *)G_malloc(misc.ntopidxclasses * sizeof(double));
    }

    for (i = 0; i < misc.ntopidxclasses; i++) {
	/* initial root zone storage deficit */
	misc.Srz[0][i] = params.Sr0;
	/* initial unsaturated zone storage */
	misc.Suz[0][i] = 0.0;
    }

    misc.S_mean = (double *)G_malloc(input.ntimesteps * sizeof(double));
    /* initial mean saturation deficit */
    misc.S_mean[0] = -params.m * log(misc.qs0 / misc.qss);

    misc.Qt = (double *)G_malloc(input.ntimesteps * sizeof(double));

    /* initial total flow */
    A1 = 0.0;
    for (i = 0; i < input.ntimesteps; i++) {
	if (i < misc.delay)
	    misc.Qt[i] = misc.qs0 * params.A;
	else if (i < misc.tc) {
	    A1 += misc.Ad[i - misc.delay];
	    misc.Qt[i] = misc.qs0 * (params.A - A1);
	} else
	    misc.Qt[i] = 0.0;
    }
}

void calculate_flows(void)
{
    int i, j, k;

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
	double f;

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
	    /* infiltration */
	    misc.f[i] = input.dt *
		    calculate_infiltration(i + 1, input.R[i] / input.dt);
	    /* infiltration excess runoff */
	    misc.fex[i] = input.R[i] - misc.f[i];
	    f = misc.f[i];
	}
	else {
	    /* no infiltration excess runoff */
	    misc.f[i] = 0.0;
	    misc.fex[i] = 0.0;
	    /* 100% of rainfall infiltrates */
	    f = input.R[i];
	}

	if (i) {
	    for (j = 0; j < misc.ntopidxclasses; j++) {
		misc.Srz[i][j] = misc.Srz[i - 1][j];
		misc.Suz[i][j] = misc.Suz[i - 1][j];
	    }
	}

	/* subsurface flow */
	misc.qs[i] = misc.qss * exp(-misc.S_mean[i] / params.m);

	for (j = 0; j < misc.ntopidxclasses; j++) {
	    double Aatb_r;

	    /* average area of a topographic index class */
	    Aatb_r = (topidxstats.Aatb_r[j] +
		      (j < misc.ntopidxclasses - 1 ? topidxstats.Aatb_r[j + 1]
		       : 0.0)) / 2.0;

	    /* saturation deficit */
	    misc.S[i][j] = misc.S_mean[i] +
		params.m * (misc.lambda - topidxstats.atb[j]);
	    if (misc.S[i][j] < 0.0)
		/* fully saturated */
		misc.S[i][j] = 0.0;

	    /* root zone storage deficit */
	    misc.Srz[i][j] -= f;
	    if (misc.Srz[i][j] < 0.0) {
		/* full storage */
		/* unsaturated zone storage */
		misc.Suz[i][j] -= misc.Srz[i][j];
		misc.Srz[i][j] = 0.0;
	    }

	    /* if there is enough unsaturated zone storage */
	    misc.ex[i][j] = 0.0;
	    if (misc.Suz[i][j] > misc.S[i][j]) {
		/* saturation excess */
		misc.ex[i][j] = misc.Suz[i][j] - misc.S[i][j];
		misc.Suz[i][j] = misc.S[i][j];
	    }

	    /* drainage from unsaturated zone */
	    misc.qv[i][j] = 0.0;
	    if (misc.S[i][j] > 0.0) {
		misc.qv[i][j] = (params.td > 0.0 ?
		       misc.Suz[i][j] /
		       (misc.S[i][j] * params.td) * input.dt
		       : -params.td * params.K0 *
		       exp(-misc.S[i][j] / params.m));
		if (misc.qv[i][j] > misc.Suz[i][j])
		    misc.qv[i][j] = misc.Suz[i][j];
		misc.Suz[i][j] -= misc.qv[i][j];
		if (misc.Suz[i][j] < ZERO)
		    misc.Suz[i][j] = 0.0;
		misc.qv[i][j] *= Aatb_r;
	    }
	    misc.qv[i][misc.ntopidxclasses] += misc.qv[i][j];

	    /* evapotranspiration from root zone storage deficit */
	    misc.Ea[i][j] = 0.0;
	    if (input.Ep[i] > 0.0) {
		misc.Ea[i][j] = input.Ep[i] *
		    (1 - misc.Srz[i][j] / params.Srmax);
		if (misc.Ea[i][j] > params.Srmax - misc.Srz[i][j])
		    misc.Ea[i][j] = params.Srmax - misc.Srz[i][j];
	    }
	    misc.Srz[i][j] += misc.Ea[i][j];

	    /* overland flow from fully saturated area */
	    misc.qo[i][j] = 0.0;
	    if (j > 0) {
		if (misc.ex[i][j] > 0.0)
		    misc.qo[i][j] = topidxstats.Aatb_r[j] *
			(misc.ex[i][j - 1] + misc.ex[i][j]) / 2.0;
		else if (misc.ex[i][j - 1] > 0.0)
		    misc.qo[i][j] = Aatb_r * misc.ex[i][j - 1] / 2.0;
	    }
	    misc.qo[i][misc.ntopidxclasses] += misc.qo[i][j];

	    /* total flow */
	    misc.qt[i][j] = misc.qo[i][j] + misc.qs[i];
	}
	/* aggregate flows over topographic index classes */
	misc.qo[i][misc.ntopidxclasses] += misc.fex[i];
	misc.qt[i][misc.ntopidxclasses] = misc.qo[i][misc.ntopidxclasses] +
		misc.qs[i];

	/* mean saturation deficit */
	misc.S_mean[i] += misc.qs[i] - misc.qv[i][misc.ntopidxclasses];
	if (i + 1 < input.ntimesteps)
	    misc.S_mean[i + 1] = misc.S_mean[i];

	/* total flow in m^3/timestep */
	for (j = 0; j < misc.tcsub; j++) {
	    k = i + j + misc.delay;
	    if (k > input.ntimesteps - 1)
		break;
	    misc.Qt[k] += misc.qt[i][misc.ntopidxclasses] * misc.Ad[j];
	}
    }

    /* mean total flow */
    misc.Qt_mean = 0.0;
    for (i = 0; i < input.ntimesteps; i++) {
	misc.Qt_mean += misc.Qt[i];
	if (!i || misc.Qt_peak < misc.Qt[i]) {
	    misc.Qt_peak = misc.Qt[i];
	    misc.tt_peak = i + 1;
	}
    }
    misc.Qt_mean /= input.ntimesteps;
}

void run_topmodel(void)
{
    initialize();
    calculate_flows();
}
