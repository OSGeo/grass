#include <math.h>


/*-
 * algorithm as241  appl. statist. (1988) 37(3):477-484.
 * produces the normal deviate z corresponding to a given lower tail
 * area of p; z is accurate to about 1 part in 10**7.
 *
 * the hash sums below are the sums of the mantissas of the coefficients.
 * they are included for use in checking transcription.
 */
double Cdhc_ppnd7(double p)
{
    static double zero = 0.0, one = 1.0, half = 0.5;
    static double split1 = 0.425, split2 = 5.0;
    static double const1 = 0.180625, const2 = 1.6;

    /* coefficients for p close to 0.5 */
    static double a[4] = { 3.3871327179, 5.0434271938e+01,
	1.5929113202e+02, 5.9109374720e+01
    };
    static double b[4] = { 0.0, 1.7895169469e+01, 7.8757757664e+01,
	6.7187563600e+01
    };

    /* hash sum ab    32.3184577772 */
    /* coefficients for p not close to 0, 0.5 or 1. */
    static double c[4] = { 1.4234372777e+00, 2.7568153900e+00,
	1.3067284816e+00, 1.7023821103e-01
    };
    static double d[3] = { 0.0, 7.3700164250e-01, 1.2021132975e-01 };

    /* hash sum cd    15.7614929821 */
    /* coefficients for p near 0 or 1. */
    static double e[4] = { 6.6579051150e+00, 3.0812263860e+00,
	4.2868294337e-01, 1.7337203997e-02
    };
    static double f[3] = { 0.0, 2.4197894225e-01, 1.2258202635e-02 };

    /* hash sum ef    19.4052910204 */
    double q, r, ret;

    q = p - half;
    if (fabs(q) <= split1) {
	r = const1 - q * q;
	ret = q * (((a[3] * r + a[2]) * r + a[1]) * r + a[0]) /
	    (((b[3] * r + b[2]) * r + b[1]) * r + one);

	return ret;;
    }
    /* else */

    if (q < zero)
	r = p;
    else
	r = one - p;

    if (r <= zero)
	return zero;

    r = sqrt(-log(r));
    if (r <= split2) {
	r = r - const2;
	ret = (((c[3] * r + c[2]) * r + c[1]) * r + c[0]) /
	    ((d[2] * r + d[1]) * r + one);
    }
    else {
	r = r - split2;
	ret = (((e[3] * r + e[2]) * r + e[1]) * r + e[0]) /
	    ((f[2] * r + f[1]) * r + one);
    }

    if (q < zero)
	ret = -ret;

    return ret;;
}


/*-
 * algorithm as241  appl. statist. (1988) 37(3):
 *
 * produces the normal deviate z corresponding to a given lower
 * tail area of p; z is accurate to about 1 part in 10**16.
 *
 * the hash sums below are the sums of the mantissas of the
 * coefficients.   they are included for use in checking
 * transcription.
 */
double ppnd16(double p)
{
    static double zero = 0.0, one = 1.0, half = 0.5;
    static double split1 = 0.425, split2 = 5.0;
    static double const1 = 0.180625, const2 = 1.6;

    /* coefficients for p close to 0.5 */
    static double a[8] = {
	3.3871328727963666080e0,
	1.3314166789178437745e+2,
	1.9715909503065514427e+3,
	1.3731693765509461125e+4,
	4.5921953931549871457e+4,
	6.7265770927008700853e+4,
	3.3430575583588128105e+4,
	2.5090809287301226727e+3
    };
    static double b[8] = { 0.0,
	4.2313330701600911252e+1,
	6.8718700749205790830e+2,
	5.3941960214247511077e+3,
	2.1213794301586595867e+4,
	3.9307895800092710610e+4,
	2.8729085735721942674e+4,
	5.2264952788528545610e+3
    };

    /* hash sum ab    55.8831928806149014439 */
    /* coefficients for p not close to 0, 0.5 or 1. */
    static double c[8] = {
	1.42343711074968357734e0,
	4.63033784615654529590e0,
	5.76949722146069140550e0,
	3.64784832476320460504e0,
	1.27045825245236838258e0,
	2.41780725177450611770e-1,
	2.27238449892691845833e-2,
	7.74545014278341407640e-4
    };
    static double d[8] = { 0.0,
	2.05319162663775882187e0,
	1.67638483018380384940e0,
	6.89767334985100004550e-1,
	1.48103976427480074590e-1,
	1.51986665636164571966e-2,
	5.47593808499534494600e-4,
	1.05075007164441684324e-9
    };

    /* hash sum cd    49.33206503301610289036 */
    /* coefficients for p near 0 or 1. */
    static double e[8] = {
	6.65790464350110377720e0,
	5.46378491116411436990e0,
	1.78482653991729133580e0,
	2.96560571828504891230e-1,
	2.65321895265761230930e-2,
	1.24266094738807843860e-3,
	2.71155556874348757815e-5,
	2.01033439929228813265e-7
    };
    static double f[8] = { 0.0,
	5.99832206555887937690e-1,
	1.36929880922735805310e-1,
	1.48753612908506148525e-2,
	7.86869131145613259100e-4,
	1.84631831751005468180e-5,
	1.42151175831644588870e-7,
	2.04426310338993978564e-15
    };

    /* hash sum ef    47.52583317549289671629 */
    double q, r, ret;

    q = p - half;
    if (fabs(q) <= split1) {
	r = const1 - q * q;
	ret = q * (((((((a[7] * r + a[6]) * r + a[5]) * r + a[4]) * r + a[3])
		     * r + a[2]) * r + a[1]) * r + a[0]) /
	    (((((((b[7] * r + b[6]) * r + b[5]) * r + b[4]) * r + b[3])
	       * r + b[2]) * r + b[1]) * r + one);

	return ret;
    }
    /* else */

    if (q < zero)
	r = p;
    else
	r = one - p;

    if (r <= zero)
	return zero;

    r = sqrt(-log(r));
    if (r <= split2) {
	r -= const2;
	ret = (((((((c[7] * r + c[6]) * r + c[5]) * r + c[4]) * r + c[3])
		 * r + c[2]) * r + c[1]) * r + c[0]) /
	    (((((((d[7] * r + d[6]) * r + d[5]) * r + d[4]) * r + d[3])
	       * r + d[2]) * r + d[1]) * r + one);
    }
    else {
	r -= split2;
	ret = (((((((e[7] * r + e[6]) * r + e[5]) * r + e[4]) * r + e[3])
		 * r + e[2]) * r + e[1]) * r + e[0]) /
	    (((((((f[7] * r + f[6]) * r + f[5]) * r + f[4]) * r + f[3])
	       * r + f[2]) * r + f[1]) * r + one);
    }

    if (q < zero)
	ret = -ret;

    return ret;
}
