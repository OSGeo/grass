#include <math.h>
#include <grass/gmath.h>


/*!
 * \fn double G_math_rand_gauss(const int seed, const double sigma)
 *
 * \brief Gaussian random number generator
 *
 * Gaussian random number generator (mean = 0.0)
 *
 * \param[in] seed
 & \param[in] sigma
 * \return double
 */

double G_math_rand_gauss(double sigma)
{
    double x, y, r2;

    do {
	/* choose x,y in uniform square (-1,-1) to (+1,+1) */
	x = -1 + 2 * G_math_rand();
	y = -1 + 2 * G_math_rand();

	/* see if it is in the unit circle */
	r2 = x * x + y * y;
    }
    while (r2 > 1.0 || r2 == 0);

    /* Box-Muller transform */
    return sigma * y * sqrt(-2.0 * log(r2) / r2);
}
