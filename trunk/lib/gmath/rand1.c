#include <stdlib.h>
#include <grass/gis.h>
#include <grass/gmath.h>


/*!
 * \fn float G_math_rand (int seed)
 *
 * \brief Random Number Generator (Uniform)
 *
 * Random number generator (Uniform Derivatives 0.0 -> 1.0)
 *
 * \param[in] seed
 * \return float
 */

float G_math_rand(void)
{
    return G_drand48();
}

/*!
 * \brief Seed the pseudo-random number generator
 *
 * \param seedval 32-bit integer used to seed the PRNG
 */

void G_math_srand(int seed)
{
    G_srand48(seed);
}

/*!
 * \brief Seed the pseudo-random number generator from the time and PID
 *
 * \return generated seed value passed to G_srand48()
 */

int G_math_srand_auto(void)
{
    return (int) G_srand48_auto();
}
