#include <grass/gis.h>
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


void read_inputs(void)
{
    FILE *fp;
    int i, j;
    double x;


    /* Read topographic index statistics file */
    fp = fopen(file.idxstats, "r");
    idxstats.atb = (double *)G_malloc(misc.nidxclass * sizeof(double));
    idxstats.Aatb_r = (double *)G_malloc(misc.nidxclass * sizeof(double));

    misc.ncell = 0;

    for (i = 0; i < misc.nidxclass && !feof(fp);) {
	get_line(fp, buf);

	if (sscanf(buf, "%lf %lf",
		   &(idxstats.atb[i]), &(idxstats.Aatb_r[i])) == 2)
	    misc.ncell += (int)idxstats.Aatb_r[i++];
    }

    misc.nidxclass = i;
    fclose(fp);

    for (i = 0; i < misc.nidxclass; i++)
	idxstats.Aatb_r[i] /= (double)misc.ncell;

    for (i = 0; i < misc.nidxclass; i++) {
	for (j = i; j < misc.nidxclass; j++) {
	    if (idxstats.atb[i] < idxstats.atb[j]) {
		x = idxstats.atb[i];
		idxstats.atb[i] = idxstats.atb[j];
		idxstats.atb[j] = x;
		x = idxstats.Aatb_r[i];
		idxstats.Aatb_r[i] = idxstats.Aatb_r[j];
		idxstats.Aatb_r[j] = x;
	    }
	}
    }


    /* Read parameters file */
    fp = fopen(file.params, "r");

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
    fp = fopen(file.input, "r");

    for (; !feof(fp);) {
	get_line(fp, buf);

	if (sscanf(buf, "%d %lf", &(input.ntimestep), &(input.dt)) == 2)
	    break;
    }

    input.R = (double *)G_malloc(input.ntimestep * sizeof(double));
    input.Ep = (double *)G_malloc(input.ntimestep * sizeof(double));

    for (i = 0; i < input.ntimestep && !feof(fp);) {
	get_line(fp, buf);

	if (sscanf(buf, "%lf %lf", &(input.R[i]), &(input.Ep[i])) == 2)
	    i++;
    }

    input.ntimestep = i;
    fclose(fp);


    /* Read Qobs file */
    if (file.Qobs) {
	fp = fopen(file.Qobs, "r");

	misc.Qobs = (double *)G_malloc(input.ntimestep * sizeof(double));

	for (i = 0; i < input.ntimestep && !feof(fp);) {
	    get_line(fp, buf);

	    if (sscanf(buf, "%lf", &(misc.Qobs[i])) == 1)
		i++;
	}

	input.ntimestep = (input.ntimestep < i ? input.ntimestep : i);
	fclose(fp);
    }


    if (!(misc.timestep > 0 && misc.timestep < input.ntimestep + 1))
	misc.timestep = 0;
    if (!(misc.idxclass > 0 && misc.idxclass < misc.nidxclass + 1))
	misc.idxclass = 0;


    return;
}


void write_outputs(void)
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


    fp = fopen(file.output, "w");

    fprintf(fp, "# r.topmodel output file for \"%s\"\n", params.name);
    fprintf(fp, "# Run time: %.4d-%.2d-%.2d %.2d:%.2d:%.2d\n",
	    ltime->tm_year, ltime->tm_mon, ltime->tm_mday,
	    ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
    fprintf(fp, "#\n");
    if (file.Qobs) {
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
    fprintf(fp, "# %-15s Number of non-NULL cells\n", "ncell:");
    fprintf(fp, "# %-15s Number of topographic index classes\n",
	    "nidxclass:");
    fprintf(fp, "# %-15s Number of delay timesteps (delay time between "
	    "rainfall and\n#\t\t\tflow response)\n", "ndelay:");
    fprintf(fp, "# %-15s Number of reach timesteps "
	    "(time of concentration)\n", "nreach:");
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

    if (misc.timestep || misc.idxclass) {
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

    if (file.Qobs) {
	fprintf(fp, "%-10s ", "Em:");
	if (!G_is_d_null_value(&misc.Em))
	    fprintf(fp, "%10.5lf\n", misc.Em);
	else
	    fprintf(fp, "Not resolved due to constant observed Q\n");
	fprintf(fp, "%-10s %10.3le\n", "Qobs_peak:", misc.Qobs_peak);
	fprintf(fp, "%-10s %10d\n", "tobs_peak:", misc.tobs_peak);
	fprintf(fp, "%-10s %10.3le\n", "Qobs_mean:", misc.Qobs_mean);
    }
    fprintf(fp, "%-10s %10.3le\n", "Qt_peak:", misc.Qt_peak);
    fprintf(fp, "%-10s %10d\n", "tt_peak:", misc.tt_peak);
    fprintf(fp, "%-10s %10.3le\n", "Qt_mean:", misc.Qt_mean);
    fprintf(fp, "%-10s %10d\n", "ncell:", misc.ncell);
    fprintf(fp, "%-10s %10d\n", "nidxclass:", misc.nidxclass);
    fprintf(fp, "%-10s %10d\n", "ndelay:", misc.ndelay);
    fprintf(fp, "%-10s %10d\n", "nreach:", misc.nreach);
    fprintf(fp, "%-10s %10.3le\n", "lnTe:", misc.lnTe);
    fprintf(fp, "%-10s %10.3le\n", "vch:", misc.vch);
    fprintf(fp, "%-10s %10.3le\n", "vr:", misc.vr);
    fprintf(fp, "%-10s %10.3le\n", "lambda:", misc.lambda);
    fprintf(fp, "%-10s %10.3le\n", "qss:", misc.qss);
    fprintf(fp, "%-10s %10.3le\n", "qs0:", misc.qs0);
    fprintf(fp, "\n");


    fprintf(fp, "%10s\n", "tch");
    for (i = 0; i < params.nch; i++)
	fprintf(fp, "%10.3le\n", misc.tch[i]);

    fprintf(fp, "%10s\n", "Ad");
    for (i = 0; i < misc.nreach; i++)
	fprintf(fp, "%10.3le\n", misc.Ad[i]);


    st = et = si = ei = 0;
    if (misc.timestep || misc.idxclass) {
	if (misc.timestep) {
	    st = misc.timestep - 1;
	    et = misc.timestep;
	}
	else {
	    st = 0;
	    et = input.ntimestep;
	}

	if (misc.idxclass) {
	    si = misc.idxclass - 1;
	    ei = misc.idxclass;
	}
	else {
	    si = 0;
	    ei = misc.nidxclass;
	}
    }

    fprintf(fp, "%10s %10s %10s %10s %10s %10s %10s", "timestep", "Qt",
	    "qt", "qo", "qs", "qv", "S_mean");
    if (params.infex)
	fprintf(fp, " %10s %10s", "f", "fex");
    fprintf(fp, "\n");

    for (i = 0; i < input.ntimestep; i++) {
	fprintf(fp, "%10d %10.3le %10.3le %10.3le %10.3le %10.3le "
		"%10.3le", i + 1, misc.Qt[i],
		misc.qt[i][misc.nidxclass],
		misc.qo[i][misc.nidxclass], misc.qs[i],
		misc.qv[i][misc.nidxclass], misc.S_mean[i]);
	if (params.infex)
	    fprintf(fp, " %10.3le %10.3le", misc.f[i], misc.fex[i]);
	fprintf(fp, "\n");
    }

    if (misc.timestep || misc.idxclass) {
	fprintf(fp, "Given ");
	if (misc.timestep)
	    fprintf(fp, "timestep: %5d", misc.timestep);
	if (misc.timestep && misc.idxclass)
	    fprintf(fp, ", ");
	if (misc.idxclass)
	    fprintf(fp, "idxclass: %5d", misc.idxclass);
	fprintf(fp, "\n");

	if (misc.timestep && !misc.idxclass) {
	    fprintf(fp, "%10s ", "idxclass");
	}
	else if (misc.idxclass && !misc.timestep) {
	    fprintf(fp, "%10s ", "timestep");
	}

	fprintf(fp, "%10s %10s %10s %10s %10s %10s %10s %10s %10s\n",
		"qt", "qo", "qs", "qv", "Srz", "Suz", "S", "Ea", "ex");

	for (i = st; i < et; i++)
	    for (j = si; j < ei; j++) {
		if (misc.timestep && !misc.idxclass) {
		    fprintf(fp, "%10d ", j + 1);
		}
		else if (misc.idxclass && !misc.timestep) {
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
