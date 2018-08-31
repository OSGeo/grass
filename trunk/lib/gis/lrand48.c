/*!
 * \file lib/gis/lrand48.c
 *
 * \brief GIS Library - Pseudo-random number generation
 *
 * (C) 2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Glynn Clements
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#else
#include <time.h>
#endif

#include <sys/types.h>
#include <unistd.h>

typedef unsigned short uint16;
typedef unsigned int uint32;
typedef signed int int32;

static uint16 x0, x1, x2;
static const uint32 a0 = 0xE66D;
static const uint32 a1 = 0xDEEC;
static const uint32 a2 = 0x5;

static const uint32 b0 = 0xB;

static int seeded;

#define LO(x) ((x) & 0xFFFFU)
#define HI(x) ((x) >> 16)

/*!
 * \brief Seed the pseudo-random number generator
 *
 * \param seedval 32-bit integer used to seed the PRNG
 */

void G_srand48(long seedval)
{
    uint32 x = (uint32) *(unsigned long *)&seedval;
    x2 = (uint16) HI(x);
    x1 = (uint16) LO(x);
    x0 = (uint16) 0x330E;
    seeded = 1;
}

/*!
 * \brief Seed the pseudo-random number generator from the time and PID
 *
 * A weak hash of the current time and PID is generated and used to
 * seed the PRNG
 *
 * \return generated seed value passed to G_srand48()
 */

long G_srand48_auto(void)
{
    unsigned long seed;
    char *grass_random_seed = getenv("GRASS_RANDOM_SEED");
    if(grass_random_seed) {
        seed = strtoull(grass_random_seed, NULL, 10);
    } else {  
        seed = (unsigned long) getpid();

#ifdef HAVE_GETTIMEOFDAY
    {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0)
	    G_fatal_error(_("gettimeofday failed: %s"), strerror(errno));
	seed += (unsigned long) tv.tv_sec;
	seed += (unsigned long) tv.tv_usec;
    }
#else
    {
	time_t t = time(NULL);
	seed += (unsigned long) t;
    }
#endif
    }

    G_srand48((long) seed);
    return (long) seed;
}

static void G__next(void)
{
    uint32 a0x0 = a0 * x0;
    uint32 a0x1 = a0 * x1;
    uint32 a0x2 = a0 * x2;
    uint32 a1x0 = a1 * x0;
    uint32 a1x1 = a1 * x1;
    uint32 a2x0 = a2 * x0;

    uint32 y0 = LO(a0x0) + b0;
    uint32 y1 = LO(a0x1) + LO(a1x0) + HI(a0x0);
    uint32 y2 = LO(a0x2) + LO(a1x1) + LO(a2x0) + HI(a0x1) + HI(a1x0);

    if (!seeded)
	G_fatal_error(_("Pseudo-random number generator not seeded"));

    x0 = (uint16) LO(y0);
    y1 += HI(y0);
    x1 = (uint16) LO(y1);
    y2 += HI(y1);
    x2 = (uint16) LO(y2);
}

/*!
 * \brief Generate an integer in the range [0, 2^31)
 *
 * \return the generated value
 */

long G_lrand48(void)
{
    uint32 r;
    G__next();
    r = ((uint32) x2 << 15) | ((uint32) x1 >> 1);
    return (long) r;
}

/*!
 * \brief Generate an integer in the range [-2^31, 2^31)
 *
 * \return the generated value
 */

long G_mrand48(void)
{
    uint32 r;
    G__next();
    r = ((uint32) x2 << 16) | ((uint32) x1);
    return (long) (int32) r;
}

/*!
 * \brief Generate a floating-point value in the range [0,1)
 *
 * \return the generated value
 */

double G_drand48(void)
{
    double r = 0.0;
    G__next();
    r += x2;
    r *= 0x10000;
    r += x1;
    r *= 0x10000;
    r += x0;
    r /= 281474976710656.0; /* 2^48 */
    return r;
}

/*

Test program

int main(int argc, char **argv)
{
    long s = (argc > 1) ? atol(argv[1]) : 0;
    int i;

    srand48(s);
    G_srand48(s);

    for (i = 0; i < 100; i++) {
	printf("%.50f %.50f\n", drand48(), G_drand48());
	printf("%lu %lu\n", lrand48(), G_lrand48());
	printf("%ld %ld\n", mrand48(), G_mrand48());
    }

    return 0;
}

*/
