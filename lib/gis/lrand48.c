/*!
 * \file lib/gis/lrand48.c
 *
 * \brief GIS Library - Pseudo-random number generation
 *
 * The generator is the standard drand48 linear congruential generator
 * X' = (A * X + B) mod 2^48 with A = 0x5DEECE66D and B = 0xB.
 *
 * Two interfaces are provided. The G_*rand48() functions draw from a
 * single generator shared by the whole program. When C11 atomic
 * operations are available, that shared state is advanced with an atomic
 * compare-and-swap and the generating functions are thread-safe: the
 * sequence of generated values for a given seed is the same as in a
 * single-threaded run. Which thread receives which value depends on
 * scheduling, so results are fully reproducible only with single-threaded
 * execution. Without C11 atomics (notably MSVC, which defines
 * __STDC_NO_ATOMICS__), the shared generator falls back to plain state
 * updates, so multi-threaded usage is safe only when compiled with C11
 * atomics. The seeding functions are not thread-safe; see G_srand48().
 *
 * The G_*rand48_r() functions instead advance a generator owned by the
 * caller. Threads holding separate states share nothing, so these need
 * neither atomics nor locks, are safe on every build, and give each
 * stream a reproducible sequence of its own. This is what parallel code
 * that must produce the same result for a given seed regardless of the
 * number of threads should use. The streams are disjoint stretches of the
 * same cycle the shared generator walks, and stream 0 starts where
 * G_srand48() does.
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
#include <stdint.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && \
    !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
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

typedef unsigned int uint32;
typedef signed int int32;

#define LCG_A  UINT64_C(0x5DEECE66D)
#define LCG_B  UINT64_C(0xB)
#define MASK48 UINT64_C(0xFFFFFFFFFFFF)

/* Advance the generator by one step. The multiplication may wrap around
 * at 2^64; that does not change the result modulo 2^48. */
static inline unsigned long long lcg_step(unsigned long long cur)
{
    return (LCG_A * cur + LCG_B) & MASK48;
}

/* Turn a seed value into a generator state the way drand48 does. */
static inline unsigned long long lcg_seed(long seedval)
{
    uint32 x = (uint32) * (unsigned long *)&seedval;

    return ((unsigned long long)x << 16) | 0x330E;
}

/* Streams are carved out of the single generator cycle by starting each
 * one STREAM_STRIDE steps after the previous, which makes them disjoint by
 * construction rather than merely unlikely to collide. The stride sets the
 * trade between how many streams exist and how far each may run before
 * reaching the next: 2^36 gives 4096 streams of about 6.9e10 draws. */
#define STREAM_STRIDE UINT64_C(0x1000000000) /* 2^36 */
#define STREAM_COUNT  (MASK48 / STREAM_STRIDE + 1)

/* Advance the generator by an arbitrary number of steps without taking
 * them one at a time. One step is the affine map x -> a * x + c, and
 * composing two such maps gives another, so the map for `steps` steps is
 * built by repeated squaring, as an integer power would be. */
static unsigned long long lcg_jump(unsigned long long x,
                                   unsigned long long steps)
{
    unsigned long long a_total = 1, c_total = 0; /* the identity map */
    unsigned long long a = LCG_A, c = LCG_B;     /* one step */

    while (steps) {
        if (steps & 1) {
            c_total = (a * c_total + c) & MASK48;
            a_total = (a * a_total) & MASK48;
        }
        /* Square the map, so a and c then describe twice as many steps. */
        c = (a * c + c) & MASK48;
        a = (a * a) & MASK48;
        steps >>= 1;
    }

    return (a_total * x + c_total) & MASK48;
}

#if LRAND48_ATOMIC
/* The whole 48-bit state is kept in one atomic integer so that it can be
 * advanced in one compare-and-swap: a successful swap is exactly one
 * generator step, giving the same sequence of states as a single-threaded
 * run. */
static _Atomic unsigned long long state;
#else
static unsigned long long state;
#endif

static int seeded;

/*!
 * \brief Seed the shared pseudo-random number generator
 *
 * This function is not thread-safe. In a multi-threaded program, call
 * `G_srand48()` once *before* starting the worker threads; it must not
 * run concurrently with another thread seeding or generating values.
 *
 * \param[in] seedval 32-bit integer used to seed the PRNG
 */
void G_srand48(long seedval)
{
#if LRAND48_ATOMIC
    atomic_store(&state, lcg_seed(seedval));
#else
    state = lcg_seed(seedval);
#endif
    seeded = 1;
}

/*!
 * \brief Seed the shared pseudo-random number generator from the time and PID
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

/* Advance the shared generator by one step and return the new state.
 * Callers derive their result from the returned value, not from the
 * shared state, so concurrent calls each get a distinct step of the
 * sequence. */
static unsigned long long G__next(void)
{
    if (!seeded)
        G_fatal_error(_("Pseudo-random number generator not seeded"));

#if LRAND48_ATOMIC
    {
        unsigned long long cur =
            atomic_load_explicit(&state, memory_order_relaxed);
        unsigned long long next;

        do {
            next = lcg_step(cur);
        } while (!atomic_compare_exchange_weak_explicit(
            &state, &cur, next, memory_order_relaxed, memory_order_relaxed));

        return next;
    }
#else
    state = lcg_step(state);
    return state;
#endif
}

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
    return (long)(G__next() >> 17);
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
    return (long)(int32)(uint32)(G__next() >> 16);
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
    /* The state is below 2^53, so the conversion to double is exact. */
    return (double)G__next() / 281474976710656.0; /* 2^48 */
}

/*!
 * \brief Seed a caller-owned pseudo-random number generator
 *
 * Sets up one of several independent generators derived from a single
 * user-visible seed. Each stream begins at a different point of the one
 * generator cycle, far enough apart that they do not run into one
 * another, so they can be drawn from concurrently without coordination.
 *
 * Give each thread, or each unit of work, its own state and its own
 * stream index. Deriving the index from the work item rather than from
 * the thread number keeps results independent of how the work is
 * scheduled, and therefore of the number of threads.
 *
 * Stream 0 begins where G_srand48() would, so code moving from the shared
 * generator to this one reproduces its existing single-threaded results.
 *
 * The caller owns the state, so this function is thread-safe as long as
 * no two threads seed the same state.
 *
 * \param[out] state generator state to seed
 * \param[in] seedval 32-bit integer used to seed the PRNG
 * \param[in] stream index identifying this stream among those derived
 *            from \p seedval, less than 4096
 */
void G_srand48_r(struct G_rand48_state *state, long seedval,
                 unsigned long stream)
{
    if (stream >= STREAM_COUNT)
        G_fatal_error(
            _("Random number stream %lu is not below the maximum of %llu"),
            stream, (unsigned long long)STREAM_COUNT);

    state->state = lcg_jump(lcg_seed(seedval), stream * STREAM_STRIDE);
}

/*!
 * \brief Generate a floating-point value in the range [0,1) from a
 *        caller-owned generator
 *
 * Thread-safe as long as no two threads share a state. Unlike
 * G_drand48(), this needs no atomics and so behaves identically on every
 * build.
 *
 * \param[in,out] state generator state, seeded with G_srand48_r()
 *
 * \return the generated value
 */
double G_drand48_r(struct G_rand48_state *state)
{
    state->state = lcg_step(state->state);
    /* The state is below 2^53, so the conversion to double is exact. */
    return (double)state->state / 281474976710656.0; /* 2^48 */
}
