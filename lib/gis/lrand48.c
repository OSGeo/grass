
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

#define LO(x) ((x) & 0xFFFFU)
#define HI(x) ((x) >> 16)

void G_srand48(long seedval)
{
    uint32 x = (uint32) *(unsigned long *)&seedval;
    x2 = (uint16) HI(x);
    x1 = (uint16) LO(x);
    x0 = (uint16) 0x330E;
}

long G_srand48_auto(void)
{
    unsigned long seed = (unsigned long) getpid();

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

    x0 = (uint16) LO(y0);
    y1 += HI(y0);
    x1 = (uint16) LO(y1);
    y2 += HI(y1);
    x2 = (uint16) LO(y2);
}

long G_lrand48(void)
{
    uint32 r;
    G__next();
    r = ((uint32) x2 << 15) | ((uint32) x1 >> 1);
    return (long) r;
}

long G_mrand48(void)
{
    uint32 r;
    G__next();
    r = ((uint32) x2 << 16) | ((uint32) x1);
    return (long) (int32) r;
}

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
