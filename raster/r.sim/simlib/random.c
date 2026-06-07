/* random.c (simlib), 20.nov.2002, JH */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/bitmap.h>
#include <grass/linkm.h>

#if defined(_OPENMP)
#include <omp.h>
#define MAX_SIMWE_THREADS 1024
static unsigned int thread_seeds[MAX_SIMWE_THREADS];
static int seeds_initialized = 0;
#endif

void simwe_rand_init(long global_seed)
{
#if defined(_OPENMP)
    int i;
    int max_threads = omp_get_max_threads();
    if (max_threads > MAX_SIMWE_THREADS) max_threads = MAX_SIMWE_THREADS;
    for (i = 0; i < max_threads; i++) {
        thread_seeds[i] = (unsigned int)(global_seed + i * 314159);
    }
    seeds_initialized = 1;
#endif
}

double simwe_rand(void)
{
#if defined(_OPENMP)
    int tid = omp_get_thread_num();
    if (tid >= MAX_SIMWE_THREADS) tid = MAX_SIMWE_THREADS - 1;
    
    if (!seeds_initialized) {
        thread_seeds[tid] = 12345 + tid; // Fallback
    }
    
    thread_seeds[tid] = (thread_seeds[tid] * 1103515245 + 12345) & 0x7fffffff;
    return (double)thread_seeds[tid] / 2147483648.0;
#else
    return G_drand48();
#endif
} /* ulec */

double gasdev(void)
{
    /* Initialized data */

    static int iset = 0;
    static double gset = .1;

    /* System generated locals */
    double ret_val;

    /* Local variables */
    double r = 0.0, vv1 = 0.0, vv2 = 0.0, fac = 0.0;

    if (iset == 0) {
        while (r >= 1. || r == 0.) {
            vv1 = simwe_rand() * 2. - 1.;
            vv2 = simwe_rand() * 2. - 1.;
            r = vv1 * vv1 + vv2 * vv2;
        }
        fac = sqrt(log(r) * -2. / r);
        gset = vv1 * fac;
        ret_val = vv2 * fac;
        iset = 1;
    }
    else {
        ret_val = gset;
        iset = 0;
    }
    return ret_val;
} /* gasdev */

void gasdev_for_paralel(double *x, double *y)
{
    double r = 0.0, vv1 = 0.0, vv2 = 0.0, fac = 0.0;

    while (r >= 1. || r == 0.) {
        vv1 = simwe_rand() * 2. - 1.;
        vv2 = simwe_rand() * 2. - 1.;
        r = vv1 * vv1 + vv2 * vv2;
    }
    fac = sqrt(log(r) * -2. / r);
    (*y) = vv1 * fac;
    (*x) = vv2 * fac;
}
