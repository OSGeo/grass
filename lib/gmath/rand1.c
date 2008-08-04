#include <grass/config.h>
#include <stdlib.h>
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

float G_math_rand(int seed)
{
#if defined(HAVE_DRAND48)
    if (seed < 0)
	srand48(-seed);

    return (float)drand48();
#else
    if (seed < 0)
	srand(-seed);

    return 1.0f * rand() / RAND_MAX;
#endif
}
