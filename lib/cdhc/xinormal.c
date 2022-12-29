#include <stdio.h>
#include <math.h>


double Cdhc_xinormal(double pee)
{
    double f0, pind, pw, px;
    static double p[5] = { -0.322232431088, -1., -0.342242088547,
	-0.0204231210245, -4.53642210148e-5
    };
    static double q[5] = { 0.099348462606, 0.588581570495, 0.531103462366,
	0.10353775285, 0.0038560700634
    };

    pind = pee;

    if (pee < 1e-10)
	return (double)-10.0;
    else if (pee >= 1.0)
	return (double)10.0;
    else if (pee == 0.5)
	return (double)0.5;
    /* else */

    if (pee > .5)
	pee--;

    pw = sqrt(log(1 / (pee * pee)));
    f0 = (((pw * q[4] + q[3]) * pw + q[2]) * pw + q[1]) * pw + q[0];
    px = pw + ((((pw * p[4] + p[3]) * pw + p[2])
		* pw + p[1]) * pw + p[0]) / f0;

    if (pind < .5)
	px = -px;

    return px;
}
