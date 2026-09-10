/*!
 * \file lib/gis/lrand48.c
 *
 * \brief GIS Library - Pseudo-random number generation
 *
 * The generator is the standard drand48 linear congruential generator
 * X' = (A * X + B) mod 2^48 with A = 0x5DEECE66D and B = 0xB.
 *
 * When C11 atomic operations are available, the generator state is
 * advanced with an atomic compare-and-swap and the generating functions
 * are thread-safe: the sequence of generated values for a given seed is
 * the same as in a single-threaded run. Which thread receives which
 * value depends on scheduling, so results are fully reproducible only
 * with single-threaded execution. Without C11 atomics (notably MSVC,
 * which defines __STDC_NO_ATOMICS__), the generator falls back to plain
 * state updates, so multi-threaded usage is safe only when compiled
 * with C11 atomics.
 *
 * The seeding functions are not thread-safe; see G_srand48().
 *
 * SPDX-FileCopyrightText: 2014-2026 GRASS Development Team
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * \authors Glynn Clements, Maris Nartiss (thread safety)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && \
    !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
#include <stdint.h>
#define LRAND48_ATOMIC 1
#else
#define LRAND48_ATOMIC 0
#endif

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

#if LRAND48_ATOMIC

/* The whole 48-bit state is kept in one atomic integer so that it can be
 * advanced in one compare-and-swap: a successful swap is exactly one
 * generator step, giving the same sequence of states as a single-threaded
 * run. The multiplication may wrap around at 2^64; that does not change
 * the result modulo 2^48. */
static atomic_uint_least64_t state;

#define LCG_A  UINT64_C(0x5DEECE66D)
#define LCG_B  UINT64_C(0xB)
#define MASK48 UINT64_C(0xFFFFFFFFFFFF)

#else

static uint16 x0, x1, x2;
static const uint32 a0 = 0xE66D;
static const uint32 a1 = 0xDEEC;
static const uint32 a2 = 0x5;

static const uint32 b0 = 0xB;

#endif /* LRAND48_ATOMIC */

static int seeded;

#define LO(x) ((x) & 0xFFFFU)
#define HI(x) ((x) >> 16)

/*!
 * \brief Seed the pseudo-random number generator
 *
 * This function is not thread-safe. In a multi-threaded program, call
 * `G_srand48()` once *before* starting the worker threads; it must not
 * run concurrently with another thread seeding or generating values.
 *
 * \param[in] seedval 32-bit integer used to seed the PRNG
 */
void G_srand48(long seedval)
{
    uint32 x = (uint32) * (unsigned long *)&seedval;

#if LRAND48_ATOMIC
    atomic_store(&state, ((uint_least64_t)x << 16) | 0x330E);
#else
    x2 = (uint16)HI(x);
    x1 = (uint16)LO(x);
    x0 = (uint16)0x330E;
#endif
    seeded = 1;
}

/*!
 * \brief Seed the pseudo-random number generator from the time and PID
 *
 * A weak hash of the current time and PID is generated and used to
 * seed the PRNG
 *
 * This function is not thread-safe. In a multi-threaded program, call
 * `G_srand48_auto()` once *before* starting the worker threads; it must
 * not run concurrently with another thread seeding or generating values.
 *
 * \return generated seed value passed to G_srand48()
 */
long G_srand48_auto(void)
{
    unsigned long seed;
    char *grass_random_seed = getenv("GRASS_RANDOM_SEED");

    if (!grass_random_seed)
        grass_random_seed = getenv("SOURCE_DATE_EPOCH");
    if (grass_random_seed) {
        seed = strtoull(grass_random_seed, NULL, 10);
    }
    else {
        seed = (unsigned long)getpid();

#ifdef HAVE_GETTIMEOFDAY
        {
            struct timeval tv;

            if (gettimeofday(&tv, NULL) < 0)
                G_fatal_error(_("gettimeofday failed: %s"), strerror(errno));
            seed += (unsigned long)tv.tv_sec;
            seed += (unsigned long)tv.tv_usec;
        }
#else
        {
            time_t t = time(NULL);

            seed += (unsigned long)t;
        }
#endif
    }

    G_srand48((long)seed);
    return (long)seed;
}

#if LRAND48_ATOMIC

/* Advance the generator by one step and return the new state. Callers
 * derive their result from the returned value, not from shared state, so
 * concurrent calls each get a distinct step of the sequence. */
static uint_least64_t G__next(void)
{
    uint_least64_t cur = atomic_load_explicit(&state, memory_order_relaxed);
    uint_least64_t next;

    if (!seeded)
        G_fatal_error(_("Pseudo-random number generator not seeded"));

    do {
        next = (LCG_A * cur + LCG_B) & MASK48;
    } while (!atomic_compare_exchange_weak_explicit(
        &state, &cur, next, memory_order_relaxed, memory_order_relaxed));

    return next;
}

#else

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

    x0 = (uint16)LO(y0);
    y1 += HI(y0);
    x1 = (uint16)LO(y1);
    y2 += HI(y1);
    x2 = (uint16)LO(y2);
}

#endif /* LRAND48_ATOMIC */

/*!
 * \brief Generate an integer in the range [0, 2^31)
 *
 * This function is thread-safe only when compiled with C11 atomics
 * (see the comment at the top of the file).
 *
 * \return the generated value
 */
long G_lrand48(void)
{
#if LRAND48_ATOMIC
    return (long)(G__next() >> 17);
#else
    uint32 r;

    G__next();
    r = ((uint32)x2 << 15) | ((uint32)x1 >> 1);
    return (long)r;
#endif
}

/*!
 * \brief Generate an integer in the range [-2^31, 2^31)
 *
 * This function is thread-safe only when compiled with C11 atomics
 * (see the comment at the top of the file).
 *
 * \return the generated value
 */
long G_mrand48(void)
{
#if LRAND48_ATOMIC
    uint32 r = (uint32)(G__next() >> 16);

    return (long)(int32)r;
#else
    uint32 r;

    G__next();
    r = ((uint32)x2 << 16) | ((uint32)x1);
    return (long)(int32)r;
#endif
}

/*!
 * \brief Generate a floating-point value in the range [0,1)
 *
 * This function is thread-safe only when compiled with C11 atomics
 * (see the comment at the top of the file).
 *
 * \return the generated value
 */
double G_drand48(void)
{
#if LRAND48_ATOMIC
    /* The state is below 2^53, so the conversion to double is exact. */
    return (double)G__next() / 281474976710656.0; /* 2^48 */
#else
    double r = 0.0;

    G__next();
    r += x2;
    r *= 0x10000;
    r += x1;
    r *= 0x10000;
    r += x0;
    r /= 281474976710656.0; /* 2^48 */
    return r;
#endif
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
