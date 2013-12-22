#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "global.h"

void get_line(FILE * fp, char *buffer)
{
    char *str;

    buffer[0] = 0;
    fscanf(fp, "%[^\n]", buffer);
    getc(fp);

    if ((str = (char *)strchr(buffer, '#')))
	*str = 0;
}

void read_input(void)
{
    char buf[1024];
    FILE *fp;
    int i, j;
    double x;

    /* Read topographic index statistics file */
    if ((fp = fopen(file.topidxstats, "r")) == NULL)
	G_fatal_error(_("Unable to open input file <%s>"), file.topidxstats);

    topidxstats.atb = NULL;
    topidxstats.Aatb_r = NULL;
    misc.ncells = 0;

    for (i = 0; !feof(fp);) {
	double atb;
	double Aatb_r;

	get_line(fp, buf);
	if (sscanf(buf, "%lf %lf", &atb, &Aatb_r) == 2) {
	    topidxstats.atb = (double *)G_realloc(topidxstats.atb,
			    (i + 1) * sizeof(double));
	    topidxstats.Aatb_r = (double *)G_realloc(topidxstats.Aatb_r,
			    (i + 1) * sizeof(double));
	    topidxstats.atb[i] = atb;
	    topidxstats.Aatb_r[i] = Aatb_r;
	    misc.ncells += (int)topidxstats.Aatb_r[i++];
	}
    }

    misc.ntopidxclasses = i;
    fclose(fp);

    for (i = 0; i < misc.ntopidxclasses; i++)
	topidxstats.Aatb_r[i] /= (double)misc.ncells;

    for (i = 0; i < misc.ntopidxclasses; i++) {
	for (j = i; j < misc.ntopidxclasses; j++) {
	    if (topidxstats.atb[i] < topidxstats.atb[j]) {
		x = topidxstats.atb[i];
		topidxstats.atb[i] = topidxstats.atb[j];
		topidxstats.atb[j] = x;
		x = topidxstats.Aatb_r[i];
		topidxstats.Aatb_r[i] = topidxstats.Aatb_r[j];
		topidxstats.Aatb_r[j] = x;
	    }
	}
    }

    /* Read parameters file */
    if ((fp = fopen(file.params, "r")) == NULL)
	G_fatal_error(_("Unable to open input file <%s>"), file.params);

    for (; !feof(fp);) {
	get_line(fp, buf);
	i = strlen(buf) - 1;
	for (; i >= 0; i--) {
	    if (buf[i] != ' ' && buf[i] != '\t') {
		buf[i + 1] = 0;
		break;
	    }
	}
	if (i >= 0)
	    break;
    }
    params.name = G_store(buf);

    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%lf", &(params.A)) == 1)
	    break;
    }
    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%lf", &(params.qs0)) == 1)
	    break;
    }
    if (params.qs0 == 0.0) {
	fclose(fp);
	G_fatal_error("parameters.qs0 cannot be 0.0");
	exit(EXIT_FAILURE);
    }
    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%lf", &(params.lnTe)) == 1)
	    break;
    }
    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%lf", &(params.m)) == 1)
	    break;
    }
    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%lf", &(params.Sr0)) == 1)
	    break;
    }
    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%lf", &(params.Srmax)) == 1)
	    break;
    }
    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%lf", &(params.td)) == 1)
	    break;
    }
    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%lf", &(params.vch)) == 1)
	    break;
    }
    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%lf", &(params.vr)) == 1)
	    break;
    }
    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%d", &(params.infex)) == 1)
	    break;
    }
    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%lf", &(params.K0)) == 1)
	    break;
    }
    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%lf", &(params.psi)) == 1)
	    break;
    }
    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%lf", &(params.dtheta)) == 1)
	    break;
    }

    params.d = NULL;
    params.Ad_r = NULL;

    for (i = 0; !feof(fp);) {
	double d;
	double Ad_r;

	get_line(fp, buf);
	if (sscanf(buf, "%lf %lf", &d, &Ad_r) == 2) {
	    params.d = (double *)G_realloc(params.d, (i + 1) * sizeof(double));
	    params.Ad_r = (double *)G_realloc(params.Ad_r,
			    (i + 1) * sizeof(double));
	    params.d[i] = d;
	    params.Ad_r[i++] = Ad_r;
	}
    }

    params.nch = i;
    fclose(fp);

    /* Read input file */
    if ((fp = fopen(file.input, "r")) == NULL)
	G_fatal_error(_("Unable to open input file <%s>"), file.input);

    for (; !feof(fp);) {
	get_line(fp, buf);
	if (sscanf(buf, "%lf", &(input.dt)) == 1)
	    break;
    }

    input.R = NULL;
    input.Ep = NULL;

    for (i = 0; !feof(fp);) {
	double R;
	double Ep;

	get_line(fp, buf);
	if (sscanf(buf, "%lf %lf", &R, &Ep) == 2) {
	    input.R = (double *)G_realloc(input.R, (i + 1) * sizeof(double));
	    input.Ep = (double *)G_realloc(input.Ep, (i + 1) * sizeof(double));
	    input.R[i] = R;
	    input.Ep[i++] = Ep;
	}
    }

    input.ntimesteps = i;
    fclose(fp);

    if (!(misc.timestep > 0 && misc.timestep < input.ntimesteps + 1))
	misc.timestep = 0;
    if (!(misc.topidxclass > 0 && misc.topidxclass < misc.ntopidxclasses + 1))
	misc.topidxclass = 0;
}

void write_output(void)
{
    FILE *fp;
    time_t tloc;
    struct tm *ltime;
    int st, et, si, ei;
    int i, j;

    time(&tloc);
    ltime = localtime(&tloc);

    ltime->tm_year += 1900;
    ltime->tm_mon++;

    if ((fp = fopen(file.output, "w")) == NULL)
	G_fatal_error(_("Unable to open output file <%s>"), file.output);

    fprintf(fp, "# r.topmodel output file for %s\n", params.name);
    fprintf(fp, "# Run time: %.4d-%.2d-%.2d %.2d:%.2d:%.2d\n",
	    ltime->tm_year, ltime->tm_mon, ltime->tm_mday,
	    ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
    fprintf(fp, "#\n");
    fprintf(fp, "# Qt_peak [m^3/timestep]:  Total peak flow\n");
    fprintf(fp, "# tt_peak [timestep]:      Peak time for total flow\n");
    fprintf(fp, "# Qt_mean [m^3/timestep]:  Mean total flow\n");
    fprintf(fp, "# lnTe [ln(m^2/timestep)]: Areal average of ln(T0)\n");
    fprintf(fp, "# vch [m/timestep]:        Main channel routing velocity\n");
    fprintf(fp, "# vr [m/timestep]:         Internal subcatchment routing velocity\n");
    fprintf(fp, "# lambda [ln(m^2)]:        Areal average of topographic index\n");
    fprintf(fp, "# qss [m/timestep]:        Subsurface flow per unit area at a soil surface\n");
    fprintf(fp, "# qs0 [m/timestep]:        Initial subsurface flow per unit area\n");
    fprintf(fp, "# ncells:                  Number of non-NULL cells\n");
    fprintf(fp, "# ntopidxclasses:          Number of topographic index classes\n");
    fprintf(fp, "# dt [h]:                  Time step\n");
    fprintf(fp, "# ntimesteps:              Number of time steps\n");
    fprintf(fp, "# nch:                     Number of routing time steps\n");
    fprintf(fp, "# nreaches:                Number of reach time steps (time of concentration)\n");
    fprintf(fp, "# ndelays:                 Delay time steps between rainfall and flow response\n");
    fprintf(fp, "#\n");
    fprintf(fp, "# tch [timestep]:          Routing time\n");
    fprintf(fp, "# Ad [m^2]:                Difference in contribution area for reach time steps\n");
    fprintf(fp, "# Qt [m^3/timestep]:       Total flow\n");
    fprintf(fp, "# qt [m/timestep]:         Total flow per unit area\n");
    fprintf(fp, "# qo [m/timestep]:         Saturation overland flow per unit area\n");
    fprintf(fp, "# qs [m/timestep]:         Subsurface flow per unit area\n");
    fprintf(fp, "# qv [m/timestep]:         Vertical flux (or drainage flux)\n");
    fprintf(fp, "# S_mean [m]:              Mean saturation deficit in the watershed\n");
    if (params.infex) {
	fprintf(fp, "# f [m/timestep]:          Infiltration rate\n");
	fprintf(fp, "# fex [m/timestep]:        Infiltration excess runoff\n");
    }

    if (misc.timestep || misc.topidxclass) {
	fprintf(fp, "#\n");
	fprintf(fp, "# Srz [m]:                 Root zone storage deficit\n");
	fprintf(fp, "# Suz [m]:                 Unsaturated (gravity drainage) zone storage\n");
	fprintf(fp, "# S [m]:                   Local saturated zone deficit due to gravity drainage\n");
	fprintf(fp, "# Ea [m/timestep]:         Actual evapotranspiration\n");
	fprintf(fp, "# ex [m/timestep]:         Excess flow from a fully saturated area per unit area\n");
    }
    fprintf(fp, "\n");

    fprintf(fp, "Qt_peak:        %10.3e\n", misc.Qt_peak);
    fprintf(fp, "tt_peak:        %10d\n", misc.tt_peak);
    fprintf(fp, "Qt_mean:        %10.3e\n", misc.Qt_mean);
    fprintf(fp, "lnTe:           %10.3e\n", misc.lnTe);
    fprintf(fp, "vch:            %10.3e\n", misc.vch);
    fprintf(fp, "vr:             %10.3e\n", misc.vr);
    fprintf(fp, "lambda:         %10.3e\n", misc.lambda);
    fprintf(fp, "qss:            %10.3e\n", misc.qss);
    fprintf(fp, "qs0:            %10.3e\n", misc.qs0);
    fprintf(fp, "ncells:         %10d\n", misc.ncells);
    fprintf(fp, "ntopidxclasses: %10d\n", misc.ntopidxclasses);
    fprintf(fp, "dt:             %10.3e\n", input.dt);
    fprintf(fp, "ntimesteps:     %10d\n", input.ntimesteps);
    fprintf(fp, "nch:            %10d\n", params.nch);
    fprintf(fp, "nreaches:       %10d\n", misc.nreaches);
    fprintf(fp, "ndelays:        %10d\n", misc.ndelays);
    fprintf(fp, "\n");

    fprintf(fp, "%10s\n", "tch");
    for (i = 0; i < params.nch; i++)
	fprintf(fp, "%10.3e\n", misc.tch[i]);
    fprintf(fp, "\n");

    fprintf(fp, "%10s\n", "Ad");
    for (i = 0; i < misc.nreaches; i++)
	fprintf(fp, "%10.3e\n", misc.Ad[i]);
    fprintf(fp, "\n");

    st = et = si = ei = 0;
    if (misc.timestep || misc.topidxclass) {
	if (misc.timestep) {
	    st = misc.timestep - 1;
	    et = misc.timestep;
	}
	else {
	    st = 0;
	    et = input.ntimesteps;
	}

	if (misc.topidxclass) {
	    si = misc.topidxclass - 1;
	    ei = misc.topidxclass;
	}
	else {
	    si = 0;
	    ei = misc.ntopidxclasses;
	}
    }

    fprintf(fp, "%10s %10s %10s %10s %10s %10s %10s",
	   "timestep", "Qt", "qt", "qo", "qs", "qv", "S_mean");
    if (params.infex)
	fprintf(fp, " %10s %10s", "f", "fex");
    fprintf(fp, "\n");

    for (i = 0; i < input.ntimesteps; i++) {
	fprintf(fp, "%10d %10.3e %10.3e %10.3e %10.3e %10.3e %10.3e",
		i + 1, misc.Qt[i],
		misc.qt[i][misc.ntopidxclasses],
		misc.qo[i][misc.ntopidxclasses], misc.qs[i],
		misc.qv[i][misc.ntopidxclasses], misc.S_mean[i]);
	if (params.infex)
	    fprintf(fp, " %10.3e %10.3e", misc.f[i], misc.fex[i]);
	fprintf(fp, "\n");
    }

    if (misc.timestep || misc.topidxclass) {
	fprintf(fp, "\n");
	fprintf(fp, "For ");
	if (misc.timestep)
	    fprintf(fp, "timestep: %5d", misc.timestep);
	if (misc.timestep && misc.topidxclass)
	    fprintf(fp, ", ");
	if (misc.topidxclass)
	    fprintf(fp, "topidxclass: %5d", misc.topidxclass);
	fprintf(fp, "\n");

	if (misc.timestep && !misc.topidxclass) {
	    fprintf(fp, "%10s ", "topidxclass");
	}
	else if (misc.topidxclass && !misc.timestep) {
	    fprintf(fp, "%10s ", "timestep");
	}

	fprintf(fp, "%10s %10s %10s %10s %10s %10s %10s %10s %10s\n",
		"qt", "qo", "qs", "qv", "Srz", "Suz", "S", "Ea", "ex");

	for (i = st; i < et; i++)
	    for (j = si; j < ei; j++) {
		if (misc.timestep && !misc.topidxclass) {
		    fprintf(fp, "%10d ", j + 1);
		}
		else if (misc.topidxclass && !misc.timestep) {
		    fprintf(fp, "%10d ", i + 1);
		}

		fprintf(fp, "%10.3e %10.3e %10.3e "
			"%10.3e %10.3e %10.3e "
			"%10.3e %10.3e %10.3e\n",
			misc.qt[i][j], misc.qo[i][j],
			misc.qs[i], misc.qv[i][j],
			misc.Srz[i][j], misc.Suz[i][j],
			misc.S[i][j], misc.Ea[i][j], misc.ex[i][j]);
	    }
    }

    fclose(fp);
}
