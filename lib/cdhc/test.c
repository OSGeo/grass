#include <stdio.h>
#include <stdlib.h>
#include <grass/cdhc.h>


int main(int argc, char **argv)
{
    double x[1000];
    double *y;
    int n = 0;

    while (scanf("%lf", &x[n++]) != EOF) ;
    n--;

    fprintf(stdout, "N=%d\n", n);
    fprintf(stdout, "*y=%ld\n", y);
    y = omnibus_moments(x, n);
    fprintf(stdout, "*y=%ld\n", y);
    y = geary_test(x, n);
    fprintf(stdout, "*y=%ld\n", y);
    y = dagostino_d(x, n);
    fprintf(stdout, "y=%g\n", y[1]);
    y = kuipers_v(x, n);
    y = watson_u2(x, n);
    y = durbins_exact(x, n);
    y = anderson_darling(x, n);
    y = cramer_von_mises(x, n);

    /* for ks and lillefors, the discrepancy seems to come
       in in computing the sum of x*x */

    y = kolmogorov_smirnov(x, n);
    y = chi_square(x, n);
    y = shapiro_wilk(x, n);
    y = shapiro_francia(x, n);
    y = shapiro_wilk_exp(x, n);
    y = cramer_von_mises_exp(x, n);
    y = kolmogorov_smirnov_exp(x, n);
    y = kuipers_v_exp(x, n);
    y = watson_u2_exp(x, n);
    y = anderson_darling_exp(x, n);
    y = chi_square_exp(x, n);

/* missing from source code:
    y = mod_maxlik_ratio(x, n);
    y = coeff_variation(x, n);
*/
    y = kotz_families(x, n);

    return EXIT_SUCCESS;
}
