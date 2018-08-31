#include <stdio.h>
#include <math.h>


double *Cdhc_omnibus_moments(double *x, int n)
{
    double diff, mean = 0., fssm, tssm, sum_cube = 0., sum_four = 0.,
	sum_sq = 0.;
    static double y[2];
    int i;

    for (i = 0; i < n; ++i)
	mean += x[i];

    mean /= n;

    for (i = 0; i < n; ++i) {
	diff = x[i] - mean;
	sum_sq += diff * diff;
	sum_cube += diff * diff * diff;
	sum_four += diff * diff * diff * diff;
    }

    /*
       fprintf (stdout,"n %d x-bar %g sum^2 %g sum^3 %g sum^4 %g \n",n,mean,sum_sq,sum_cube,sum_four);
     */
    tssm = sqrt((double)n) * sum_cube / pow(sum_sq, 1.5);
    fssm = n * sum_four / (sum_sq * sum_sq);

#ifdef NOISY
    fprintf(stdout,
	    "          TESTS OF COMPOSITE DISTRIBUTIONAL HYPOTHESES\n");
    fprintf(stdout, "  TEST1  TSM    =%10.4f   FSM    =%10.4f\n", tssm, fssm);
#endif /* NOISY */

    y[0] = tssm;
    y[1] = fssm;

    return y;
}
