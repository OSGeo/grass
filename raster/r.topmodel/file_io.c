#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <grass/gis.h>
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

    return;
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

    for (i = 0, j = 0; !feof(fp);) {
	double atb;
	double Aatb_r;

	get_line(fp, buf);

	if (sscanf(buf, "%lf %lf", &atb, &Aatb_r) == 2) {
	    topidxstats.atb = (double *)G_realloc(topidxstats.atb,
			    (j + 1) * sizeof(double));
	    topidxstats.Aatb_r = (double *)G_realloc(topidxstats.Aatb_r,
			    (j + 1) * sizeof(double));
	    topidxstats.atb[j] = atb;
	    topidxstats.Aatb_r[j] = Aatb_r;
	    misc.ncells += (int)topidxstats.Aatb_r[j++];
	}
    }

    misc.ntopidxclasses = j;

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

	if (sscanf(buf, "%lf %lf %lf %lf %lf %lf %lf %lf",
		   &(params.qs0), &(params.lnTe),
		   &(params.m), &(params.Sr0),
		   &(params.Srmax), &(params.td),
		   &(params.vch), &(params.vr)) == 8)
	    break;
    }

    if (params.qs0 == 0.0) {
	fclose(fp);
	G_fatal_error("parameters.qs0 can not be 0.0");
	exit(EXIT_FAILURE);
    }

    for (; !feof(fp);) {
	get_line(fp, buf);

	if (sscanf(buf, "%d %lf %lf %lf",
		   &(params.infex), &(params.K0),
		   &(params.psi), &(params.dtheta)) == 4)
	    break;
    }

    for (; !feof(fp);) {
	get_line(fp, buf);

	if (sscanf(buf, "%d", &(params.nch)) == 1)
	    break;
    }

    params.d = (double *)G_malloc(params.nch * sizeof(double));
    params.Ad_r = (double *)G_malloc(params.nch * sizeof(double));

    for (i = 0; i < params.nch && !feof(fp);) {
	get_line(fp, buf);

	if (sscanf(buf, "%lf %lf", &(params.d[i]), &(params.Ad_r[i])) == 2)
	    i++;
    }

    params.nch = i;
    fclose(fp);


    /* Read input file */
    if ((fp = fopen(file.input, "r")) == NULL)
	G_fatal_error(_("Unable to open input file <%s>"), file.input);

    for (; !feof(fp);) {
	get_line(fp, buf);

	if (sscanf(buf, "%d %lf", &(input.ntimesteps), &(input.dt)) == 2)
	    break;
    }

    input.R = (double *)G_malloc(input.ntimesteps * sizeof(double));
    input.Ep = (double *)G_malloc(input.ntimesteps * sizeof(double));

    for (i = 0; i < input.ntimesteps && !feof(fp);) {
	get_line(fp, buf);

	if (sscanf(buf, "%lf %lf", &(input.R[i]), &(input.Ep[i])) == 2)
	    i++;
    }

    input.ntimesteps = i;
    fclose(fp);

    /* Read Qobs file */
    if (file.obsflow) {
	if ((fp = fopen(file.obsflow, "r")) == NULL)
	    G_fatal_error(_("Unable to open input file <%s>"), file.obsflow);

	misc.Qobs = (double *)G_malloc(input.ntimesteps * sizeof(double));

	for (i = 0; i < input.ntimesteps && !feof(fp);) {
	    get_line(fp, buf);

	    if (sscanf(buf, "%lf", &(misc.Qobs[i])) == 1)
		i++;
	}

	input.ntimesteps = (input.ntimesteps < i ? input.ntimesteps : i);
	fclose(fp);
    }


    if (!(misc.timestep > 0 && misc.timestep < input.ntimesteps + 1))
	misc.timestep = 0;
    if (!(misc.topidxclass > 0 && misc.topidxclass < misc.ntopidxclasses + 1))
	misc.topidxclass = 0;

    return;
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

    fprintf(fp, "# r.topmodel output file for \"%s\"\n", params.name);
    fprintf(fp, "# Run time: %.4d-%.2d-%.2d %.2d:%.2d:%.2d\n",
	    ltime->tm_year, ltime->tm_mon, ltime->tm_mday,
	    ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
    fprintf(fp, "#\n");
    if (file.obsflow) {
	fprintf(fp, "# %-15s Model efficiency\n", "Em:");
	fprintf(fp, "# %-15s Peak observed Q\n"
		"# %77s\n", "Qobs_peak:", "[m^3/timestep]");
	fprintf(fp, "# %-15s Peak time for observed Q\n"
		"# %77s\n", "tobs_peak:", "[timestep]");
	fprintf(fp, "# %-15s Mean observed Q\n"
		"# %77s\n", "Qobs_mean:", "[m^3/timestep]");
    }
    fprintf(fp, "# %-15s Peak simulated Q\n"
	    "# %77s\n", "Qt_peak:", "[m^3/timestep]");
    fprintf(fp, "# %-15s Peak time for simulated Q\n"
	    "# %77s\n", "tt_peak:", "[timestep]");
    fprintf(fp, "# %-15s Mean simulated Q\n"
	    "# %77s\n", "Qt_mean:", "[m^3/timestep]");
    fprintf(fp, "# %-15s Number of non-NULL cells\n", "ncells:");
    fprintf(fp, "# %-15s Number of topographic index classes\n",
	    "ntopidxclasses:");
    fprintf(fp, "# %-15s Number of delay timesteps (delay time between "
	    "rainfall and\n#\t\t\tflow response)\n", "ndelays:");
    fprintf(fp, "# %-15s Number of reach timesteps "
	    "(time of concentration)\n", "nreaches:");
    fprintf(fp, "# %-15s Areal average of ln(T0) = ln(Te)\n"
	    "# %77s\n", "lnTe:", "[ln(m^2/timestep)]");
    fprintf(fp, "# %-15s Main channel routing velocity\n"
	    "# %77s\n", "vch:", "[m/timestep]");
    fprintf(fp, "# %-15s Internal subcatchment routing velocity\n"
	    "# %77s\n", "vr:", "[m/timestep]");
    fprintf(fp, "# %-15s Areal average of topographic index\n"
	    "# %77s\n", "lambda:", "[ln(m^2)]");
    fprintf(fp, "# %-15s Subsurface flow per unit area at a soil surface\n"
	    "# %77s\n", "qss:", "[m/timestep]");
    fprintf(fp, "# %-15s Initial subsurface flow per unit area\n"
	    "# %77s\n", "qs0:", "[m/timestep]");
    fprintf(fp, "#\n");
    fprintf(fp, "# %-15s Routing timestep\n"
	    "# %77s\n", "tch:", "[timestep]");
    fprintf(fp, "# %-15s Difference in contribution area for each reach "
	    "timestep\n" "# %77s\n", "Ad:", "[m^2]");
    fprintf(fp, "# %-15s Total flow\n" "# %77s\n", "Qt:", "[m^3/timestep]");
    fprintf(fp, "# %-15s Total flow per unit area\n"
	    "# %77s\n", "qt:", "[m/timestep]");
    fprintf(fp, "# %-15s Saturation overland flow per unit area\n"
	    "# %77s\n", "qo:", "[m/timestep]");
    fprintf(fp, "# %-15s Subsurface flow per unit area\n"
	    "# %77s\n", "qs:", "[m/timestep]");
    fprintf(fp, "# %-15s Vertical flux (or drainage flux)\n"
	    "# %77s\n", "qv:", "[m/timestep]");
    fprintf(fp, "# %-15s Mean saturation deficit in the watershed\n"
	    "# %77s\n", "S_mean:", "[m]");
    if (params.infex) {
	fprintf(fp, "# %-15s Infiltration rate\n"
		"# %30s\n", "f:", "[m/timestep]");
	fprintf(fp, "# %-15s Infiltration excess runoff\n"
		"# %77s\n", "fex:", "[m/timestep]");
    }

    if (misc.timestep || misc.topidxclass) {
	fprintf(fp, "#\n");
	fprintf(fp, "# %-15s Root zone storage deficit\n"
		"# %77s\n", "Srz:", "[m]");
	fprintf(fp, "# %-15s Unsaturated (gravity drainage) zone "
		"storage\n" "# %77s\n", "Suz:", "[m]");
	fprintf(fp, "# %-15s Local saturated zone deficit due to "
		"gravity drainage\n" "# %77s\n", "S:", "[m]");
	fprintf(fp, "# %-15s Actual evapotranspiration\n"
		"# %77s\n", "Ea:", "[m/timestep]");
	fprintf(fp, "# %-15s Excess flow from a fully saturated "
		"area per unit area\n" "# %77s\n", "ex:", "[m/timestep]");
    }

    fprintf(fp, "\n");

    if (file.obsflow) {
	fprintf(fp, "%-16s ", "Em:");
	if (!Rast_is_d_null_value(&misc.Em))
	    fprintf(fp, "%10.5lf\n", misc.Em);
	else
	    fprintf(fp, "Not resolved due to constant observed Q\n");
	fprintf(fp, "%-16s %10.3le\n", "Qobs_peak:", misc.Qobs_peak);
	fprintf(fp, "%-16s %10d\n", "tobs_peak:", misc.tobs_peak);
	fprintf(fp, "%-16s %10.3le\n", "Qobs_mean:", misc.Qobs_mean);
    }
    fprintf(fp, "%-16s %10.3le\n", "Qt_peak:", misc.Qt_peak);
    fprintf(fp, "%-16s %10d\n", "tt_peak:", misc.tt_peak);
    fprintf(fp, "%-16s %10.3le\n", "Qt_mean:", misc.Qt_mean);
    fprintf(fp, "%-16s %10d\n", "ncells:", misc.ncells);
    fprintf(fp, "%-16s %10d\n", "ntopidxclasses:", misc.ntopidxclasses);
    fprintf(fp, "%-16s %10d\n", "ndelays:", misc.ndelays);
    fprintf(fp, "%-16s %10d\n", "nreaches:", misc.nreaches);
    fprintf(fp, "%-16s %10.3le\n", "lnTe:", misc.lnTe);
    fprintf(fp, "%-16s %10.3le\n", "vch:", misc.vch);
    fprintf(fp, "%-16s %10.3le\n", "vr:", misc.vr);
    fprintf(fp, "%-16s %10.3le\n", "lambda:", misc.lambda);
    fprintf(fp, "%-16s %10.3le\n", "qss:", misc.qss);
    fprintf(fp, "%-16s %10.3le\n", "qs0:", misc.qs0);
    fprintf(fp, "\n");

    fprintf(fp, "%10s\n", "tch");
    for (i = 0; i < params.nch; i++)
	fprintf(fp, "%10.3le\n", misc.tch[i]);
    fprintf(fp, "\n");

    fprintf(fp, "%10s\n", "Ad");
    for (i = 0; i < misc.nreaches; i++)
	fprintf(fp, "%10.3le\n", misc.Ad[i]);
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

    fprintf(fp, "%10s %10s %10s %10s %10s %10s %10s", "timestep", "Qt",
	    "qt", "qo", "qs", "qv", "S_mean");
    if (params.infex)
	fprintf(fp, " %10s %10s", "f", "fex");
    fprintf(fp, "\n");

    for (i = 0; i < input.ntimesteps; i++) {
	fprintf(fp, "%10d %10.3le %10.3le %10.3le %10.3le %10.3le "
		"%10.3le", i + 1, misc.Qt[i],
		misc.qt[i][misc.ntopidxclasses],
		misc.qo[i][misc.ntopidxclasses], misc.qs[i],
		misc.qv[i][misc.ntopidxclasses], misc.S_mean[i]);
	if (params.infex)
	    fprintf(fp, " %10.3le %10.3le", misc.f[i], misc.fex[i]);
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

		fprintf(fp, "%10.3le %10.3le %10.3le "
			"%10.3le %10.3le %10.3le "
			"%10.3le %10.3le %10.3le\n",
			misc.qt[i][j], misc.qo[i][j],
			misc.qs[i], misc.qv[i][j],
			misc.Srz[i][j], misc.Suz[i][j],
			misc.S[i][j], misc.Ea[i][j], misc.ex[i][j]);
	    }
    }

    fclose(fp);

    return;
}
