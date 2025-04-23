/* functions to classify sorted arrays of doubles and fill a vector of
 * classbreaks */

#include <stdbool.h>

#include <grass/glocale.h>
#include <grass/arraystats.h>

int AS_option_to_algorithm(const struct Option *option)
{
    if (G_strcasecmp(option->answer, "int") == 0)
        return CLASS_INTERVAL;
    if (G_strcasecmp(option->answer, "std") == 0)
        return CLASS_STDEV;
    if (G_strcasecmp(option->answer, "qua") == 0)
        return CLASS_QUANT;
    if (G_strcasecmp(option->answer, "equ") == 0)
        return CLASS_EQUIPROB;
    if (G_strcasecmp(option->answer, "dis") == 0)
        return CLASS_DISCONT;

    G_fatal_error(_("Unknown algorithm '%s'"), option->answer);
}

double AS_class_apply_algorithm(int algo, const double data[], int nrec,
                                int *nbreaks, double classbreaks[])
{
    double finfo = 0.0;

    switch (algo) {
    case CLASS_INTERVAL:
        finfo = AS_class_interval(data, nrec, *nbreaks, classbreaks);
        break;
    case CLASS_STDEV:
        finfo = AS_class_stdev(data, nrec, *nbreaks, classbreaks);
        break;
    case CLASS_QUANT:
        finfo = AS_class_quant(data, nrec, *nbreaks, classbreaks);
        break;
    case CLASS_EQUIPROB:
        finfo = AS_class_equiprob(data, nrec, nbreaks, classbreaks);
        break;
    case CLASS_DISCONT:
        finfo = AS_class_discont(data, nrec, *nbreaks, classbreaks);
        break;
    default:
        break;
    }

    if (finfo == 0)
        G_fatal_error(_("Classification algorithm failed"));

    return finfo;
}

int AS_class_interval(const double data[], int count, int nbreaks,
                      double classbreaks[])
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

double AS_class_stdev(const double data[], int count, int nbreaks,
                      double classbreaks[])
{
    struct GASTATS stats;
    int i;
    int nbclass;
    double scale = 1.0;

    AS_basic_stats(data, count, &stats);

    nbclass = nbreaks + 1;

    if (nbclass % 2 ==
        1) { /* number of classes is uneven so we center middle class on mean */

        /* find appropriate fraction of stdev for step */
        i = 1;
        while (i) {
            if (((stats.mean + stats.stdev * scale / 2) +
                     (stats.stdev * scale * (nbclass / 2 - 1)) >
                 stats.max) ||
                ((stats.mean - stats.stdev * scale / 2) -
                     (stats.stdev * scale * (nbclass / 2 - 1)) <
                 stats.min))
                scale = scale / 2;
            else
                i = 0;
        }

        /* classbreaks below the mean */
        for (i = 0; i < nbreaks / 2; i++)
            classbreaks[i] = (stats.mean - stats.stdev * scale / 2) -
                             stats.stdev * scale * (nbreaks / 2 - (i + 1));
        /* classbreaks above the mean */
        for (; i < nbreaks; i++)
            classbreaks[i] = (stats.mean + stats.stdev * scale / 2) +
                             stats.stdev * scale * (i - nbreaks / 2);
    }
    else { /* number of classes is even so mean is a classbreak */

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
        for (; i < nbreaks; i++)
            classbreaks[i] =
                stats.mean + stats.stdev * scale * (i - nbreaks / 2);
    }

    return (scale);
}

int AS_class_quant(const double data[], int count, int nbreaks,
                   double classbreaks[])
{
    int i, step;

    step = count / (nbreaks + 1);

    for (i = 0; i < nbreaks; i++)
        classbreaks[i] = data[step * (i + 1)];

    return (1);
}

int AS_class_equiprob(const double data[], int count, int *nbreaks,
                      double classbreaks[])
{
    int i, j;
    double *lequi; /*Vector of scale factors for probabilities of the normal
                      distribution */
    struct GASTATS stats;
    int nbclass;

    nbclass = *nbreaks + 1;

    lequi = G_malloc(*nbreaks * sizeof(double));

    /* The following values come from the normal distribution and will be used
     * as: classbreak[i] = (lequi[i] * stdev) + mean;
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
        G_fatal_error(
            _("Equiprobable classbreaks currently limited to 10 classes"));
    }

    AS_basic_stats(data, count, &stats);

    /* Check if any of the classbreaks would fall outside of the range min-max
     */
    j = 0;
    for (i = 0; i < *nbreaks; i++) {
        if ((lequi[i] * stats.stdev + stats.mean) >= stats.min &&
            (lequi[i] * stats.stdev) + stats.mean <= stats.max) {
            j++;
        }
    }

    if (j < (*nbreaks)) {
        G_warning(
            _("There are classbreaks outside the range min-max. Number of "
              "classes reduced to %i, but using probabilities for %i classes."),
            j + 1, *nbreaks + 1);
        for (i = 0; i < j; i++)
            classbreaks[i] = 0.0;
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

double AS_class_discont(const double data[], int count, int nbreaks,
                        double classbreaks[])
{
    int i, j;
    double chi2 = 1000.0;

    /* get the number of values */
    int n = count;

    int nbclass = nbreaks + 1;

    int *num = G_calloc((nbclass + 2), sizeof(int));
    int *no = G_calloc((nbclass + 1), sizeof(int));
    double *zz = G_malloc((nbclass + 1) * sizeof(double));
    double *xn = G_malloc((n + 1) * sizeof(double));
    double *co = G_malloc((nbclass + 1) * sizeof(double));

    /* We copy the array of values to x, in order to be able
       to standardize it */
    double *x = G_malloc((n + 1) * sizeof(double));
    x[0] = 0.0;
    xn[0] = 0.0;

    double min = data[0];
    double max = data[count - 1];
    for (i = 1; i <= n; i++)
        x[i] = data[i - 1];

    double rangemax = max - min;
    double rangemin = rangemax;

    for (i = 2; i <= n; i++) {
        if (x[i] != x[i - 1] && x[i] - x[i - 1] < rangemin)
            rangemin = x[i] - x[i - 1]; /* rangemin = minimal distance */
    }

    /* STANDARDIZATION
     * and creation of the number vector (xn) */

    for (i = 1; i <= n; i++) {
        x[i] = (x[i] - min) / rangemax;
        xn[i] = i / (double)n;
    }
    double xlim = rangemin / rangemax;
    rangemin = rangemin / 2.0;
    /* Searching for the limits */
    num[1] = n;

    /* Loop through possible solutions */
    for (i = 1; i <= nbclass; i++) {
        double dmax = 0.0;
        int nmax = 0;
        int nf = 0; /* End number */

        /* Loop through classes */
        for (j = 1; j <= i; j++) {
            double a = 0.0, b = 0.0, c = 0.0, d = 0.0;
            int nd = nf; /* Start number */

            nf = num[j];
            co[j] = 10e37;
            AS_eqdrt(x, xn, nd, nf, &a, &b, &c);
            double den = sqrt(pow(b, 2) + 1.0);
            nd++;
            /* Loop through observations */
            for (int k = nd; k <= nf; k++) {
                if (fabs(c) >= GRASS_EPSILON)
                    d = fabs(x[k] - c);
                else
                    d = fabs((-1.0 * b * x[k]) + xn[k] - a) / den;

                if (x[k] - x[nd] < xlim)
                    continue;
                if (x[nf] - x[k] < xlim)
                    continue;
                if (d <= dmax)
                    continue;
                dmax = d;
                nmax = k;
            }
            nd--;
            if (fabs(x[nf] - x[nd]) > GRASS_EPSILON)
                co[j] = (xn[nf] - xn[nd]) / (x[nf] - x[nd]);
        }
        for (j = 1; j <= i; j++) {
            no[j] = num[j];
            zz[j] = x[num[j]] * rangemax + min;
            if (j == i)
                continue;
            if (co[j] > co[j + 1]) {
                zz[j] = zz[j] + rangemin;
                continue;
            }
            else {
                zz[j] = zz[j] - rangemin;
                no[j] = no[j] - 1;
            }
        }
        int im = i - 1;
        if (im != 0) {
            for (j = 1; j <= im; j++) {
                int ji = i + 1 - j;
                no[ji] -= no[ji - 1];
            }
        }
        if (nmax == 0) {
            break;
        }

        int jj = 0;
        int nff = i + 2;
        bool do_reset = true;
        for (j = 1; j <= i; j++) {
            jj = nff - j;
            if (num[jj - 1] < nmax) {
                num[jj] = nmax;
                do_reset = false;
                break;
            }
            num[jj] = num[jj - 1];
        }
        if (do_reset) {
            num[1] = nmax;
            jj = 1;
        }
        int no1 = (int)((xn[num[jj]] - xn[num[jj - 1]]) * n);
        int no2 = (int)((xn[num[jj + 1]] - xn[num[jj]]) * n);
        double f = (xn[num[jj + 1]] - xn[num[jj - 1]]) /
                   (x[num[jj + 1]] - x[num[jj - 1]]);
        f *= n;
        double xt1 = (x[num[jj]] - x[num[jj - 1]]) * f;
        double xt2 = (x[num[jj + 1]] - x[num[jj]]) * f;
        if (fabs(xt1 * xt2) <= GRASS_EPSILON) {
            if (fabs(xt2) > GRASS_EPSILON) {
                xt2 = rangemin / 2.0 / rangemax * f;
                xt1 = xt1 - xt2;
            }
            else {
                xt1 = rangemin / 2.0 / rangemax * f;
                xt2 = xt2 - xt1;
            }
        }

        /* calculate chi-square to indicate statistical significance of new
         * class, i.e. how probable would it be that the new class could be the
         * result of purely random choice */
        double ch = pow((double)((no1 - no2) - (xt1 - xt2)), 2) / (xt1 + xt2);
        if (chi2 > ch)
            chi2 = ch;
    }

    /*  Fill up classbreaks of i <nbclass classes */
    for (j = 1; j < nbclass; j++)
        classbreaks[j - 1] = zz[j];

    G_free(co);
    G_free(no);
    G_free(num);
    G_free(x);
    G_free(xn);
    G_free(zz);

    return (chi2);
}

int AS_class_frequencies(const double data[], int count, int nbreaks,
                         double classbreaks[], int frequencies[])
{
    int i, j;

    /* min = data[0];
       max = data[count - 1]; */
    /* count cases in all classes, except for last class */
    i = 0;
    for (j = 0; j < nbreaks; j++) {
        while (data[i] <= classbreaks[j]) {
            frequencies[j]++;
            i++;
        }
    }

    /*Now count cases in last class */
    for (; i < count; i++) {
        frequencies[nbreaks]++;
    }

    return (1);
}
