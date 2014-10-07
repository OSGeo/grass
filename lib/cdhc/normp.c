#include <math.h>


/*-
 *     SUBROUTINE Cdhc_normp(Z, P, Q, PDF)
 *
 *     Normal distribution probabilities accurate to 1.e-15.
 *     Z = no. of standard deviations from the mean.
 *     P, Q = probabilities to the left & right of Z.   P + Q = 1.
 *     PDF = the probability density.
 *
 *     Based upon algorithm 5666 for the error function, from:
 *     Hart, J.F. et al, 'Computer Approximations', Wiley 1968
 *
 *     Programmer: Alan Miller
 *
 *     Latest revision - 30 March 1986
 *
 */

/* Conversion to C by James Darrell McCauley, 24 September 1994 */

double Cdhc_normp(double z)
{
    static double p[7] = { 220.2068679123761, 221.2135961699311,
	112.079291497870, 33.91286607838300, 6.37396220353165,
	0.7003830644436881, 0.352624965998910e-1
    };
    static double q[8] = { 440.4137358247522, 793.8265125199484,
	637.3336333788311, 296.5642487796737, 86.78073220294608,
	16.06417757920695, 1.755667163182642, 0.8838834764831844e-1
    };
    static double cutoff = 7.071, root2pi = 2.506628274631001;
    double zabs, expntl;
    double pee, queue, pdf;

    zabs = fabs(z);

    if (zabs > 37.0) {
	pdf = 0.0;
	if (z > 0.0) {
	    pee = 1.0;
	    queue = 0.0;
	}
	else {
	    pee = 0.0;
	    queue = 1.0;
	}

	return pee;
    }

    expntl = exp(-0.5 * zabs * zabs);
    pdf = expntl / root2pi;

    if (zabs < cutoff)
	pee = expntl * ((((((p[6] * zabs + p[5]) * zabs + p[4])
			   * zabs + p[3]) * zabs + p[2]) * zabs +
			 p[1]) * zabs + p[0])
	    / (((((((q[7] * zabs + q[6]) * zabs + q[5]) * zabs + q[4])
		  * zabs + q[3]) * zabs + q[2]) * zabs + q[1]) * zabs + q[0]);
    else
	pee = pdf / (zabs + 1.0 / (zabs + 2.0 / (zabs + 3.0 / (zabs + 4.0 /
							       (zabs +
								0.65)))));

    if (z < 0.0)
	queue = 1.0 - pee;
    else {
	queue = pee;
	pee = 1.0 - queue;
    }

    return pee;
}
