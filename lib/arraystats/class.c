/* functions to classify sorted arrays of doubles and fill a vector of classbreaks */

#include <grass/glocale.h>
#include <grass/arraystats.h>

double class_apply_algorithm(char *algo, double *data, int nrec, int *nbreaks,
			     double *classbreaks)
{
    double finfo = 0.0;

    if (G_strcasecmp(algo, "int") == 0)
	finfo = class_interval(data, nrec, *nbreaks, classbreaks);
    else if (G_strcasecmp(algo, "std") == 0)
	finfo = class_stdev(data, nrec, *nbreaks, classbreaks);
    else if (G_strcasecmp(algo, "qua") == 0)
	finfo = class_quant(data, nrec, *nbreaks, classbreaks);
    else if (G_strcasecmp(algo, "equ") == 0)
	finfo = class_equiprob(data, nrec, nbreaks, classbreaks);
    else if (G_strcasecmp(algo, "dis") == 0)
	    /*	finfo = class_discont(data, nrec, *nbreaks, classbreaks); disabled because of bugs */
        G_fatal_error(_("Discont algorithm currently not available because of bugs"));
    else
	G_fatal_error(_("%s: Unknown algorithm"), algo);

    if (finfo == 0)
	G_fatal_error(_("%s: Error in classification algorithm"), algo);

    return finfo;
}

int class_interval(double *data, int count, int nbreaks, double *classbreaks)
{
    double min, max;
    double step;
    int i = 0;

    min = data[0];
    max = data[count - 1];

    step = (max - min) / (nbreaks + 1);

    for (i = 0; i < nbreaks; i++)
	classbreaks[i] = min + (step * (i + 1));

    return (1);
}

double class_stdev(double *data, int count, int nbreaks, double *classbreaks)
{
    struct GASTATS stats;
    int i;
    int nbclass;
    double scale = 1.0;

    basic_stats(data, count, &stats);

    nbclass = nbreaks + 1;

    if (nbclass % 2 == 1) {	/* number of classes is uneven so we center middle class on mean */

	/* find appropriate fraction of stdev for step */
	i = 1;
	while (i) {
	    if (((stats.mean + stats.stdev * scale / 2) +
		 (stats.stdev * scale * (nbclass / 2 - 1)) > stats.max) ||
		((stats.mean - stats.stdev * scale / 2) -
		 (stats.stdev * scale * (nbclass / 2 - 1)) < stats.min))
		scale = scale / 2;
	    else
		i = 0;
	}

	/* classbreaks below the mean */
	for (i = 0; i < nbreaks / 2; i++)
	    classbreaks[i] =
		(stats.mean - stats.stdev * scale / 2) -
		stats.stdev * scale * (nbreaks / 2 - (i + 1));
	/* classbreaks above the mean */
	for (i = i; i < nbreaks; i++)
	    classbreaks[i] =
		(stats.mean + stats.stdev * scale / 2) +
		stats.stdev * scale * (i - nbreaks / 2);

    }
    else {			/* number of classes is even so mean is a classbreak */

	/* decide whether to use 1*stdev or 0.5*stdev as step */
	i = 1;
	while (i) {
	    if (((stats.mean) + (stats.stdev * scale * (nbclass / 2 - 1)) >
		 stats.max) ||
		((stats.mean) - (stats.stdev * scale * (nbclass / 2 - 1)) <
		 stats.min))
		scale = scale / 2;
	    else
		i = 0;
	}

	/* classbreaks below the mean and on the mean */
	for (i = 0; i <= nbreaks / 2; i++)
	    classbreaks[i] =
		stats.mean - stats.stdev * scale * (nbreaks / 2 - i);
	/* classbreaks above the mean */
	for (i = i; i < nbreaks; i++)
	    classbreaks[i] =
		stats.mean + stats.stdev * scale * (i - nbreaks / 2);
    }

    return (scale);
}

int class_quant(double *data, int count, int nbreaks, double *classbreaks)
{
    int i, step;

    step = count / (nbreaks + 1);

    for (i = 0; i < nbreaks; i++)
	classbreaks[i] = data[step * (i + 1)];

    return (1);
}


int class_equiprob(double *data, int count, int *nbreaks, double *classbreaks)
{
    int i, j;
    double *lequi;		/*Vector of scale factors for probabilities of the normal distribution */
    struct GASTATS stats;
    int nbclass;

    nbclass = *nbreaks + 1;

    lequi = G_malloc(*nbreaks * sizeof(double));

    /*The following values come from the normal distribution and 
     * will be used as: 
     * classbreak[i] = (lequi[i] * stdev) + mean;
     */

    if (nbclass < 3) {
	lequi[0] = 0;
    }
    else if (nbclass == 3) {
	lequi[0] = -0.43076;
	lequi[1] = 0.43076;
    }
    else if (nbclass == 4) {
	lequi[0] = -0.6745;
	lequi[1] = 0;
	lequi[2] = 0.6745;
    }
    else if (nbclass == 5) {
	lequi[0] = -0.8416;
	lequi[1] = -0.2533;
	lequi[2] = 0.2533;
	lequi[3] = 0.8416;
    }
    else if (nbclass == 6) {
	lequi[0] = -0.9676;
	lequi[1] = -0.43076;
	lequi[2] = 0;
	lequi[3] = 0.43076;
	lequi[4] = 0.9676;
    }
    else if (nbclass == 7) {
	lequi[0] = -1.068;
	lequi[1] = -0.566;
	lequi[2] = -0.18;
	lequi[3] = 0.18;
	lequi[4] = 0.566;
	lequi[5] = 1.068;
    }
    else if (nbclass == 8) {
	lequi[0] = -1.1507;
	lequi[1] = -0.6745;
	lequi[2] = -0.3187;
	lequi[3] = 0;
	lequi[4] = 0.3187;
	lequi[5] = 0.6745;
	lequi[6] = 1.1507;
    }
    else if (nbclass == 9) {
	lequi[0] = -1.2208;
	lequi[1] = -0.7648;
	lequi[2] = -0.4385;
	lequi[3] = -0.1397;
	lequi[4] = 0.1397;
	lequi[5] = 0.4385;
	lequi[6] = 0.7648;
	lequi[7] = 1.2208;
    }
    else if (nbclass == 10) {
	lequi[0] = -1.28155;
	lequi[1] = -0.84162;
	lequi[2] = -0.5244;
	lequi[3] = -0.25335;
	lequi[4] = 0;
	lequi[5] = 0.25335;
	lequi[6] = 0.5244;
	lequi[7] = 0.84162;
	lequi[8] = 1.28155;
    }
    else {
	G_fatal_error
	    ("Equiprobable classbreaks currently limited to 10 classes");
    }

    basic_stats(data, count, &stats);

    /*check if any of the classbreaks would fall outside of the range min-max */
    j = 0;
    for (i = 0; i < *nbreaks; i++) {
	if ((lequi[i] * stats.stdev + stats.mean) >= stats.min &&
	    (lequi[i] * stats.stdev) + stats.mean <= stats.max) {
	    j++;
	}
    }

    if (j < (*nbreaks)) {
	G_warning(_("There are classbreaks outside the range min-max. Number of classes reduced to %i, but using probabilities for %i classes."),
		  j + 1, *nbreaks + 1);
	G_realloc(classbreaks, j * sizeof(double));
	for (i = 0; i < j; i++)
	    classbreaks[i] = 0;
    }

    j = 0;
    for (i = 0; i < *nbreaks; i++) {
	if ((lequi[i] * stats.stdev + stats.mean) >= stats.min &&
	    (lequi[i] * stats.stdev) + stats.mean <= stats.max) {
	    classbreaks[j] = lequi[i] * stats.stdev + stats.mean;
	    j++;
	}
    }

    *nbreaks = j;

    G_free(lequi);
    return (1);
}

/* FixMe: there seems to a problem with array overflow, probably due to the fact that the code was ported from fortran which has 1-based arrays*/
double class_discont(double *data, int count, int nbreaks,
		     double *classbreaks)
{
    int *num, nbclass;
    double *no, *zz, *nz, *xn, *co;
    double *x;			/* Vecteur des observations standardisées */
    int i, j, k;
    double min = 0, max = 0, rangemax = 0;
    int n = 0;
    double rangemin = 0, xlim = 0;
    double dmax = 0.0, d2 = 0.0, dd = 0.0, p = 0.0;
    int nf = 0, nmax = 0;
    double *abc;
    int nd = 0;
    double den = 0, d = 0;
    int im = 0, ji = 0;
    int tmp = 0;
    int nff = 0, jj = 0, no1 = 0, no2 = 0;
    double f = 0, xt1 = 0, xt2 = 0, chi2 = 1000.0, xnj_1 = 0, xj_1 = 0;


    /*get the number of values */
    n = count;

    nbclass = nbreaks + 1;

    num = G_malloc((nbclass + 1) * sizeof(int));
    no = G_malloc((nbclass + 1) * sizeof(double));
    zz = G_malloc((nbclass + 1) * sizeof(double));
    nz = G_malloc(3 * sizeof(double));
    xn = G_malloc((n + 1) * sizeof(double));
    co = G_malloc((nbclass + 1) * sizeof(double));

    /* We copy the array of values to x, in order to be able to standardize it */
    x = G_malloc((n + 1) * sizeof(double));
    x[0] = n;
    xn[0] = 0;

    min = data[0];
    max = data[count - 1];
    for (i = 1; i <= n; i++)
	x[i] = data[i - 1];

    rangemax = max - min;
    rangemin = rangemax;

    for (i = 2; i <= n; i++) {
	if (x[i] != x[i - 1] && x[i] - x[i - 1] < rangemin)
	    rangemin = x[i] - x[i - 1];	/* rangemin = minimal distance */
    }

    /* STANDARDIZATION 
     * and creation of the number vector (xn) */

    for (i = 1; i <= n; i++) {
	x[i] = (x[i] - min) / rangemax;
	xn[i] = i / (double)n;
    }
    xlim = rangemin / rangemax;
    rangemin = rangemin / 2.0;

    /* Searching for the limits */
    num[1] = n;
    abc = G_malloc(3 * sizeof(double));

    /*     Loop through possible solutions */
    for (i = 1; i <= nbclass; i++) {
	nmax = 0;
	dmax = 0.0;
	d2 = 0.0;
	nf = 0;			/*End number */

	/*           Loop through classes */
	for (j = 1; j <= i; j++) {
	    nd = nf;		/*Start number */
	    nf = num[j];
	    co[j] = 10e37;
	    eqdrt(x, xn, nd, nf, abc);
	    den = sqrt(pow(abc[1], 2) + 1.0);
	    nd++;
	    /*              Loop through observations */
	    for (k = nd; k <= nf; k++) {
		if (abc[2] == 0.0)
		    d = fabs((-1.0 * abc[1] * x[k]) + xn[k] - abc[0]) / den;
		else
		    d = fabs(x[k] - abc[2]);
		d2 += pow(d, 2);
		if (x[k] - x[nd] < xlim)
		    continue;
		if (x[nf] - x[k] < xlim)
		    continue;
		if (d <= dmax)
		    continue;
		dmax = d;
		nmax = k;
	    }
	    nd--;		/* A VERIFIER! */
	    if (x[nf] != x[nd]) {
		if (nd != 0)
		    co[j] = (xn[nf] - xn[nd]) / (x[nf] - x[nd]);
		else
		    co[j] = (xn[nf]) / (x[nf]);	/* A VERIFIER! */
	    }
	}
	if (i == 1)
	    dd = d2;
	p = d2 / dd;
	for (j = 1; j <= i; j++) {
	    no[j] = num[j];
	    zz[j] = x[num[j]] * rangemax + min;
	    if (j == i)
		continue;
	    if (co[j] > co[j + 1]) {
		zz[j] = zz[j] + rangemin;
		continue;
	    }
	    zz[j] = zz[j] - rangemin;
	    no[j] = no[j] - 1;
	}
	im = i - 1;
	if (im != 0.0) {
	    for (j = 1; j <= im; j++) {
		ji = i + 1 - j;
		no[ji] -= no[ji - 1];
	    }
	}
	if (nmax == 0) {
	    break;
	}
	nff = i + 2;
	tmp = 0;
	for (j = 1; j <= i; j++) {
	    jj = nff - j;
	    if (num[jj - 1] < nmax) {
		num[jj] = nmax;
		tmp = 1;
		break;
	    }
	    num[jj] = num[jj - 1];
	}
	if (tmp == 0) {
	    num[1] = nmax;
	    jj = 1;
	}
	if (jj == 1) {
	    xnj_1 = 0;
	    xj_1 = 0;
	}
	else {
	    xnj_1 = xn[num[jj - 1]];
	    xj_1 = x[num[jj - 1]];
	}
	no1 = (xn[num[jj]] - xnj_1) * n;
	no2 = (xn[num[jj + 1]] - xn[num[jj]]) * n;
	f = (xn[num[jj + 1]] - xnj_1) / (x[num[jj + 1]] - xj_1);
	f *= n;
	xt1 = (x[num[jj]] - xj_1) * f;
	xt2 = (x[num[jj + 1]] - x[num[jj]]) * f;
	if (xt2 == 0) {
	    xt2 = rangemin / 2.0 / rangemax * f;
	    xt1 -= xt2;
	}
	else if (xt1 * xt2 == 0) {
	    xt1 = rangemin / 2.0 / rangemax * f;
	    xt2 -= xt1;
	}

	/* calculate chi-square to indicate statistical significance of new class, i.e. how probable would it be that the new class could be the result of purely random choice */
	if (chi2 > pow((double)((no1 - no2) - (xt1 - xt2)), 2) / (xt1 + xt2))
	    chi2 = pow((double)((no1 - no2) - (xt1 - xt2)), 2) / (xt1 + xt2);

    }

    /*  Fill up classbreaks of i <=nbclass classes */
    for (j = 0; j <= (i - 1); j++)
	classbreaks[j] = zz[j + 1];

    return (chi2);
}

int class_frequencies(double *data, int count, int nbreaks,
		      double *classbreaks, int *frequencies)
{
    int i, j;
    double min, max;

    min = data[0];
    max = data[count - 1];
    /* count cases in all classes, except for last class */
    i = 0;
    for (j = 0; j < nbreaks; j++) {
	while (data[i] <= classbreaks[j]) {
	    frequencies[j]++;
	    i++;
	}
    }

    /*Now count cases in last class */
    for (i = i; i < count; i++) {
	frequencies[nbreaks]++;
    }

    return (1);
}
