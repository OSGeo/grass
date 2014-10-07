
/*-Algorithm AS 66
 * The Normal Integral, by I. D. Hill, 1973.
 * Applied Statistics 22(3):424-427.
 *
 * Translation to C by James Darrell McCauley, mccauley@ecn.purdue.edu.
 *
 * Calculates the upper or lower tail area of the standardized normal
 * curve corresponding to any given argument.
 *
 * x - the argument value
 * upper:  1 -> the area from x to \infty
 *         0 -> the area from -\infty to x
 *
 * Notes:
 * The constant LTONE should be set to the value at which the
 * lower tail area becomes 1.0 to the accuracy of the machine.
 * LTONE=(n+9)/3 gives the required value accurately enough, for a
 * machine that produces n decimal digits in its real numbers.
 *
 * The constant UTZERO should be set to the value at which the upper
 * tail area becomes 0.0 to the accuracy of the machine. This may be
 * taken as the value such that exp(-0.5 * UTZERO * UTZERO) /
 * (UTZERO * sqrt(2*M_PI)) is just greater than the smallest allowable
 * real numbers.
 */

#include <math.h>


#define LTONE 7.0
#define UTZERO 18.66


double Cdhc_alnorm(double x, int upper)
{
    double ret, z, y;
    int up;

    up = upper;
    z = x;

    if (x < 0.0) {
	up = up == 0 ? 1 : 0;
	z = -x;
    }

    if (!(z <= LTONE || (up == 1 && z <= UTZERO)))
	ret = 0.0;
    else {
	y = 0.5 * z * z;
	if (z <= 1.28)
	    ret = 0.5 - z * (0.398942280444 - 0.399903438504 * y /
			     (y + 5.75885480458 - 29.8213557808 /
			      (y + 2.62433121679 + 48.6959930692 /
			       (y + 5.92885724438))));
	else
	    ret = 0.398942280385 * exp(-y) /
		(z - 3.8052e-8 + 1.00000615302 /
		 (z + 3.98064794e-4 + 1.98615381364 /
		  (z - 0.151679116635 + 5.29330324926 /
		   (z + 4.8385912808 - 15.1508972451 /
		    (z + 0.742380924027 + 30.789933034 /
		     (z + 3.99019417011))))));
    }

    if (up == 0)
	ret = 1.0 - ret;

    return ret;
}
