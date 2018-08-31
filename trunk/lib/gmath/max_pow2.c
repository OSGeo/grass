#include <grass/gis.h>
#include <grass/gmath.h>


/*!
 * \fn long G_math_max_pow2 (const long n)
 *
 * \brief Finds least power of 2 >= <b>n</b>
 *
 * Finds least power of 2 >= <b>n</b>.
 *
 * \param[in] n
 * \return long
 */

long G_math_max_pow2(const long n)
{
    long p2, n1;

    n1 = n >> 1;
    p2 = 1;
    while (n1 > 0) {
	n1 >>= 1;
	p2 <<= 1;
    }
    if (p2 < n)
	p2 <<= 1;

    return (p2);
}


/*!
 * \fn long G_math_min_pow2 (const long n)
 *
 * \brief Finds largest power of 2 <= <b>n</b>
 *
 * Finds largest power of 2 <= <b>n</b>.
 *
 * \param[in] n
 * \return long
 */

long G_math_min_pow2(const long n)
{
    long p2, n1;

    n1 = n >> 1;
    p2 = 1;
    while (n1 > 0) {
	n1 >>= 1;
	p2 <<= 1;
    }

    return (p2);
}
