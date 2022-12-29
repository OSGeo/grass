#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "kappa.h"
#include "local_proto.h"


void calc_kappa(void)
{
    int i, j;
    int s, l;
    int nl;
    size_t ns;
    double *pi, *pj, *pii, p0, pC;
    double kp, vkp, *kpp;
    double obs, inter1, inter2;
    long total;
    FILE *fd;

    /* initialize */
    s = 0;
    l = -1;
    nl = nlayers;
    ns = nstats;
    obs = 0;
    inter1 = inter2 = 0;
    p0 = pC = 0;

    if (output == NULL)
	fd = stdout;
    else if ((fd = fopen(output, "a")) == NULL) {
	G_fatal_error(_("Cannot open file <%s> to write kappa and relevant parameters"),
		      output);
	return;
    }

    total = count_sum(&s, l);

    /* calculate the parameters of the kappa-calculation */
    pi = (double *)G_calloc(ns, sizeof(double));
    pj = (double *)G_calloc(ns, sizeof(double));
    pii = (double *)G_calloc(ns, sizeof(double));
    kpp = (double *)G_calloc(ns, sizeof(double));

    for (i = 0; i < ncat; i++) {
	for (j = 0; j < ns; j++) {
	    if (Gstats[j].cats[0] == rlst[i])
		pi[i] += Gstats[j].count;

	    if (Gstats[j].cats[1] == rlst[i])
		pj[i] += Gstats[j].count;

	    if ((Gstats[j].cats[0] == Gstats[j].cats[1]) &&
		(Gstats[j].cats[0] == rlst[i]))
		pii[i] += Gstats[j].count;
	}
	obs += pii[i];
    }

    for (i = 0; i < ncat; i++) {
	pi[i] = pi[i] / total;
	pj[i] = pj[i] / total;
	pii[i] = pii[i] / total;
	p0 += pii[i];
	pC += pi[i] * pj[i];
    }

    for (i = 0; i < ncat; i++)
	if ((pi[i] == 0) || (pj[i] == 0))
	    kpp[i] = -999;
	else
	    kpp[i] = (pii[i] - pi[i] * pj[i]) / (pi[i] - pi[i] * pj[i]);

    /* print out the comission and omission accuracy, and conditional kappa */
    fprintf(fd, "\nCats\t%% Comission\t%% Omission\tEstimated Kappa\n");
    for (i = 0; i < ncat; i++)
	if ((kpp[i] == -999) && (i != 0))
	    fprintf(fd, "%ld\tNA\t\tNA\t\tNA\n", rlst[i]);
	else
	    fprintf(fd, "%ld\t%f\t%f\t%f\n",
		    rlst[i], 100 * (1 - pii[i] / pi[i]),
		    100 * (1 - pii[i] / pj[i]), kpp[i]);
    fprintf(fd, "\n");

    for (i = 0; i < ncat; i++) {
	inter1 += pii[i] * pow(((1 - pC) - (1 - p0) * (pi[i] + pj[i])), 2.);
    }
    for (j = 0; j < ns; j++) {
	if (Gstats[j].cats[0] != Gstats[j].cats[1])
	    inter2 += Gstats[j].count *
		pow((pi[Gstats[j].cats[0] - 1] + pj[Gstats[j].cats[1] - 1]),
		    2.) / total;
    }
    kp = (p0 - pC) / (1 - pC);
    vkp = (inter1 + pow((1 - p0), 2.) * inter2 -
	   pow((p0 * pC - 2 * pC + p0), 2.)) / pow((1 - pC), 4.) / total;
    fprintf(fd, "Kappa\t\tKappa Variance\n");
    fprintf(fd, "%f\t%f\n", kp, vkp);

    fprintf(fd, "\nObs Correct\tTotal Obs\t%% Observed Correct\n");
    fprintf(fd, "%ld\t\t%ld\t\t%f\n", (long)obs, total, (100. * obs / total));
    if (output != NULL)
	fclose(fd);
    G_free(pi);
    G_free(pj);
    G_free(pii);
    G_free(kpp);
    /* print labels for categories of maps */
    prt_label();
}
