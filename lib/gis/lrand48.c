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
 * The G_random_*() functions instead advance generators owned by the
 * program, one struct G_random_state per unit of work. The program keeps
 * a state to one thread at a time; the functions then need neither
 * atomics nor locks, behave identically on every build, and give each
 * unit a sequence fixed by the seed and the unit's number, not by the
 * thread that draws it. The library places the units within the
 * span, the first 2^46 draws after the seed, a quarter of the generator's
 * cycle: a layout gives every unit of every batch a stream of stride draws
 * in it, and a state is put at the start of its unit's stream. See \ref
 * gislib_random_streams for the model.
 *
 * SPDX-FileCopyrightText: 2014-2026 GRASS Development Team
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * \authors Glynn Clements, Maris Nartiss, Vaclav Petras
 */

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef unsigned short uint16;
typedef unsigned int uint32;
typedef signed int int32;

#define LCG_A                UINT64_C(0x5DEECE66D)
#define LCG_B                UINT64_C(0xB)
#define MASK48               UINT64_C(0xFFFFFFFFFFFF)

/* The multiplier has order 2^46 modulo 2^48, so two states 2^46, 2^47 or
 * 3 * 2^46 draws apart give values which differ by a constant for as long
 * as they run. The layouts therefore place all their units within the
 * first 2^46 draws after the seed, the span. */
#define LCG_SPAN             (UINT64_C(1) << 46)

/* The most units a layout of the whole span takes; see
 * G_random_init_layout() for why. */
#define WHOLE_SPAN_MAX_UNITS (INT64_C(1) << 20)

#if LRAND48_ATOMIC

/* The whole 48-bit state is kept in one atomic integer so that it can be
 * advanced in one compare-and-swap: a successful swap is exactly one
 * generator step, giving the same sequence of states as a single-threaded
 * run. The multiplication may wrap around at 2^64; that does not change
 * the result modulo 2^48. */
static atomic_uint_least64_t state;

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

/* The seeds the generator tells apart: G_srand48() uses the low 32 bits of
 * the seed, and this range names every such seed once, as a signed or an
 * unsigned 32-bit value. */
static int seed_in_range(int64_t seed)
{
    return seed >= -(INT64_C(1) << 31) && seed <= (INT64_C(1) << 32) - 1;
}

/* Read a seed from an environment variable. An unset or empty variable
 * gives no seed, so that the caller can try the next source. */
static int seed_from_environment(const char *name, int64_t *seed)
{
    const char *text = getenv(name);
    char *end;
    int64_t value;

    if (!text || !*text)
        return 0;
    errno = 0;
    value = strtoll(text, &end, 10);
    if (errno == ERANGE)
        G_fatal_error(
            _("Random number seed %s from %s is too large in magnitude "
              "to be read"),
            text, name);
    if (end == text || *end != '\0')
        G_fatal_error(_("Random number seed %s from %s is not an integer"),
                      text, name);
    if (!seed_in_range(value)) {
        int64_t reduced = (int64_t)((uint64_t)value & 0xFFFFFFFF);

        G_warning(_("Random number seed %s from %s is used as %" PRId64 ", "
                    "its low 32 bits"),
                  text, name, reduced);
        value = reduced;
    }
    *seed = value;
    return 1;
}

/*!
 * \brief Generate a seed for a random number generator
 *
 * The seed is the value of the environment variable GRASS_RANDOM_SEED, or
 * of SOURCE_DATE_EPOCH when GRASS_RANDOM_SEED is not set or empty, and
 * otherwise a weak hash of the current time and process ID. A value from
 * the environment must be a decimal integer within the range of int64_t,
 * with nothing after it (leading white space and a sign are allowed);
 * anything else is a fatal error naming the variable. A value from -2^31
 * to 2^32 - 1 is returned as it is, and a value outside that range is
 * reduced to its low 32 bits, between 0 and 2^32 - 1, with a warning. The
 * result is therefore a seed G_random_state_from_seed(), the layout
 * functions and G_srand48() accept. Record it, for example in the history of
 * the output map, so that the run can be repeated with it as the seed.
 *
 * The hash of the time and process ID lies between 0 and 2^32 - 1. Two
 * calls in one process within the same microsecond, or within the same
 * second on systems without gettimeofday(), return the same value, and
 * two processes can get the same value too.
 *
 * The function reads the environment and the clock and changes no
 * generator; it may issue a warning or end with a fatal error.
 *
 * \return the seed
 */
int64_t G_random_generate_seed(void)
{
    int64_t given;
    uint64_t seed;

    if (seed_from_environment("GRASS_RANDOM_SEED", &given) ||
        seed_from_environment("SOURCE_DATE_EPOCH", &given))
        return given;

    seed = (uint64_t)getpid();

#ifdef HAVE_GETTIMEOFDAY
    {
        struct timeval tv;

        if (gettimeofday(&tv, NULL) < 0)
            G_fatal_error(_("gettimeofday failed: %s"), strerror(errno));
        seed += (uint64_t)tv.tv_sec;
        seed += (uint64_t)tv.tv_usec;
    }
#else
    {
        time_t t = time(NULL);

        seed += (uint64_t)t;
    }
#endif

    return (int64_t)(seed & 0xFFFFFFFF);
}

/*!
 * \brief Seed the pseudo-random number generator from the time and PID
 *
 * The seed is what G_random_generate_seed() returns: the value of
 * GRASS_RANDOM_SEED or SOURCE_DATE_EPOCH, or a weak hash of the current
 * time and PID.
 *
 * This function is not thread-safe. In a multi-threaded program, call
 * `G_srand48_auto()` once *before* starting the worker threads; it must
 * not run concurrently with another thread seeding or generating values.
 *
 * \return the seed passed to G_srand48(), between -2^31 and 2^32 - 1;
 *         where long has 32 bits, a value of 2^31 or more comes back
 *         negative, which G_srand48() and G_random_state_from_seed() read
 *         as the same seed
 */
long G_srand48_auto(void)
{
    int64_t seed = G_random_generate_seed();

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

/* Multiply two values modulo 2^48. The unsigned multiplication may wrap
 * around modulo 2^64. This is intentional and does not change the result:
 * since 2^48 divides 2^64, masking the wrapped product to 48 bits gives
 * the same result as computing the full product modulo 2^48. */
static uint64_t mul48(uint64_t a, uint64_t b)
{
    /* coverity[integer_overflow] */
    return (a * b) & MASK48;
}

/* Advance a generator of the program's own by one step. All generator
 * arithmetic is modulo 2^48; mul48() performs multiplication with
 * intentional unsigned wraparound as described above. */
static uint64_t lcg_step(uint64_t x)
{
    return (mul48(LCG_A, x) + LCG_B) & MASK48;
}

/* Turn a seed into a generator state the way G_srand48() does: only the
 * low 32 bits are used, so a negative seed gives the state of its two's
 * complement 32-bit value. */
static uint64_t lcg_seed(int64_t seed)
{
    return (((uint64_t)seed & 0xFFFFFFFF) << 16) | 0x330E;
}

/* Advance the generator by an arbitrary number of steps without taking
 * them one at a time. One step is the affine map x -> a * x + c, and
 * composing two such maps gives another, so the map for `steps` steps is
 * built by repeated squaring, as an integer power would be. All
 * multiplication is modulo 2^48; see mul48(). */
static uint64_t lcg_jump(uint64_t x, uint64_t steps)
{
    uint64_t a_total = 1, c_total = 0; /* the identity map */
    uint64_t a = LCG_A, c = LCG_B;     /* one step */

    while (steps) {
        if (steps & 1) {
            c_total = (mul48(a, c_total) + c) & MASK48;
            a_total = mul48(a, a_total);
        }
        /* Square the map, so a and c then describe twice as many steps. */
        c = (mul48(a, c) + c) & MASK48;
        a = mul48(a, a);
        steps >>= 1;
    }

    return (mul48(a_total, x) + c_total) & MASK48;
}

/* The generator tells seeds apart by their low 32 bits only. Seeds read
 * as signed or as unsigned 32-bit values are accepted, so -1 and
 * 4294967295 are both accepted and are the same seed; anything else is
 * rejected rather than silently losing its high bits. */
static void check_seed(int64_t seed)
{
    if (!seed_in_range(seed))
        G_fatal_error(_("Random number seed %" PRId64 " is outside the range "
                        "from -2147483648 to 4294967295 the generator can use"),
                      seed);
}

static void check_units(int64_t units)
{
    if (units <= 0)
        G_fatal_error(_("The number of units of a random number layout must "
                        "be positive, not %" PRId64),
                      units);
}

/* The draws from the start of one batch to the start of the next: those of
 * one batch, units * stride, or one more when that number is even. With an
 * odd distance, as with an odd stride between units, the same draw of
 * different batches is as unrelated as the generator allows; an even
 * distance would relate those draws, the more simply the more often 2
 * divides it. */
static uint64_t batch_distance(const struct G_random_layout *layout)
{
    return ((uint64_t)layout->units * (uint64_t)layout->stride) | 1;
}

/* Fill in a layout of units of the given stride, all of one batch first,
 * then those of the next batch. The batches that fit are those whose last
 * stream ends within the span. Callers must check beforehand that
 * units * stride does not overflow. */
static void fill_layout(struct G_random_layout *layout, int64_t seed,
                        int64_t units, int64_t stride)
{
    uint64_t draws = (uint64_t)units * (uint64_t)stride;

    layout->start = lcg_seed(seed);
    layout->units = units;
    layout->stride = stride;
    layout->batches = 0;
    if (draws <= LCG_SPAN)
        layout->batches =
            (int64_t)((LCG_SPAN - draws) / batch_distance(layout) + 1);
    layout->whole_span = false;
}

/*!
 * \brief Seed a pseudo-random number generator of the program's own
 *
 * Puts the state at the seed: it then produces the sequence G_srand48()
 * followed by G_drand48() produces for the same seed, so code moving from
 * the shared generator to one of its own reproduces its existing results.
 * Use it for a single sequence, which then has the whole span to itself;
 * for one sequence per unit of work, use a layout and
 * G_random_state_for_unit().
 *
 * A seed outside -2^31 to 2^32 - 1 is a fatal error. A negative seed
 * means its two's complement 32-bit value, as for G_srand48().
 *
 * Thread-safe as long as no two threads seed the same state.
 *
 * \param[out] state generator state to seed
 * \param[in] seed seed, from -2^31 to 2^32 - 1
 */
void G_random_state_from_seed(struct G_random_state *state, int64_t seed)
{
    check_seed(seed);
    state->state = lcg_seed(seed);
}

/*!
 * \brief Initialize a layout whose units draw an exact number of values
 *
 * The span is cut into streams of exactly \p draws_per_unit draws, one per
 * unit, placed one after another from the seed. Unit u of batch 0 therefore
 * draws what a single sequence from G_random_state_from_seed() draws at
 * positions u * \p draws_per_unit to (u + 1) * \p draws_per_unit - 1, so
 * the units together reproduce a serial run whatever order they are
 * processed in. Further batches follow, see G_random_state_for_batch().
 * A unit which draws more than \p draws_per_unit values runs into the
 * next unit's stream; when the number of draws is only bounded, use
 * G_random_init_layout_bounded().
 *
 * The stride is \p draws_per_unit. G_random_layout_length() returns it,
 * and G_random_layout_batches() returns the batches that fit, which is 0
 * when \p units * \p draws_per_unit draws do not fit into the span. See \ref
 * gislib_random_streams.
 *
 * A seed outside -2^31 to 2^32 - 1, a number of units or of draws which
 * is not positive, or a product of the two beyond the range of int64_t
 * is a fatal error.
 *
 * \param[out] layout layout to initialize
 * \param[in] seed seed, see G_random_state_from_seed()
 * \param[in] units number of units of work, positive
 * \param[in] draws_per_unit number of values each unit draws, positive
 */
void G_random_init_layout_exact(struct G_random_layout *layout, int64_t seed,
                                int64_t units, int64_t draws_per_unit)
{
    check_seed(seed);
    check_units(units);
    if (draws_per_unit <= 0)
        G_fatal_error(_("The number of random numbers a unit draws must be "
                        "positive, not %" PRId64),
                      draws_per_unit);
    if (units > INT64_MAX / draws_per_unit)
        G_fatal_error(_("A random number layout of %" PRId64 " units of "
                        "%" PRId64 " draws each is too large (the number of "
                        "draws must not exceed %" PRId64 ")"),
                      units, draws_per_unit, INT64_MAX);
    fill_layout(layout, seed, units, draws_per_unit);
}

/*!
 * \brief Initialize a layout whose units draw at most a number of values
 *
 * As G_random_init_layout_exact(), but each unit may draw any number of
 * values up to \p max_draws: the streams are \p max_draws draws long, or
 * \p max_draws + 1 when \p max_draws is even. With an odd stride, units
 * 2^i apart in number start an odd multiple of 2^i draws apart, so the
 * differences of their states are fixed in the low i + 2 bits only; an
 * even stride would add its own power of two to that count.
 *
 * The stride is \p max_draws rounded up to odd. G_random_layout_length()
 * returns it, and G_random_layout_batches() returns the batches that fit,
 * which is 0 when \p units times the stride draws do not fit into the span. See
 * \ref gislib_random_streams.
 *
 * A seed outside -2^31 to 2^32 - 1, a number of units or a bound which is
 * not positive, or a product of the units and the stride beyond the range
 * of int64_t is a fatal error.
 *
 * \param[out] layout layout to initialize
 * \param[in] seed seed, see G_random_state_from_seed()
 * \param[in] units number of units of work, positive
 * \param[in] max_draws most values any unit draws, positive
 */
void G_random_init_layout_bounded(struct G_random_layout *layout, int64_t seed,
                                  int64_t units, int64_t max_draws)
{
    int64_t stride;

    check_seed(seed);
    check_units(units);
    if (max_draws <= 0)
        G_fatal_error(_("The most random numbers a unit draws must be "
                        "positive, not %" PRId64),
                      max_draws);
    /* INT64_MAX is odd, so an even bound can be rounded up. */
    stride = max_draws % 2 == 0 ? max_draws + 1 : max_draws;
    if (units > INT64_MAX / stride)
        G_fatal_error(_("A random number layout of %" PRId64 " units of at "
                        "most %" PRId64 " draws each is too large (the number "
                        "of units times the bound rounded up to odd must not "
                        "exceed %" PRId64 ")"),
                      units, max_draws, INT64_MAX);
    fill_layout(layout, seed, units, stride);
}

/*!
 * \brief Initialize a layout which gives the units the whole span
 *
 * For units whose number of draws is not known in advance. The span is
 * divided into parts, as many as there are units, or one more when that
 * number is even. The stride is 2^46 / parts rounded down, and then down
 * to odd when there is more than one part, and unit u starts u * stride
 * draws after the seed. The odd number of parts keeps the unit halfway or
 * a quarter of the way along from starting 2^45 or 2^44 draws after unit
 * 0, distances at which this generator's values relate (see
 * \ref gislib_random_streams): the unit nearest to 2^45 starts at least
 * 0.48 of a stride away from it and the unit nearest to 2^44 at least
 * 0.24, so a unit meets such a relation only after drawing about a
 * quarter of its stride. The odd stride puts units 2^i apart in number an
 * odd multiple of 2^i draws apart, as in a bounded layout. A single unit
 * keeps the whole span of 2^46 draws, as a state from
 * G_random_state_from_seed() does. The layout holds a single batch.
 *
 * The number of units is limited to 2^20: the rounded stride loses up to
 * two draws per unit, which accumulate along the span, so beyond that
 * count the units nearest to 2^45 and 2^44 would drift toward them. A
 * bounded layout serves any number of units.
 *
 * The stride is the length of a part. G_random_layout_length() returns
 * it, and G_random_layout_batches() returns 1.
 *
 * A seed outside -2^31 to 2^32 - 1, or a number of units which is not
 * positive or above 2^20, is a fatal error.
 *
 * \param[out] layout layout to initialize
 * \param[in] seed seed, see G_random_state_from_seed()
 * \param[in] units number of units of work, from 1 to 2^20
 */
void G_random_init_layout(struct G_random_layout *layout, int64_t seed,
                          int64_t units)
{
    uint64_t parts, stride;

    check_seed(seed);
    check_units(units);
    if (units > WHOLE_SPAN_MAX_UNITS)
        G_fatal_error(_("A random number layout of the whole span takes at "
                        "most %" PRId64 " units, not %" PRId64 "; a bounded "
                        "layout serves any number of units"),
                      WHOLE_SPAN_MAX_UNITS, units);
    parts = (uint64_t)units | 1;
    stride = LCG_SPAN / parts;
    if (parts > 1 && stride % 2 == 0)
        stride--;
    layout->start = lcg_seed(seed);
    layout->units = units;
    layout->stride = (int64_t)stride;
    layout->batches = 1;
    layout->whole_span = true;
}

/*!
 * \brief Return the number of batches that fit into the span
 *
 * \param[in] layout an initialized layout
 *
 * \return the batches that fit, 1 for a layout of the whole span, 0 when
 *         one batch of the layout does not fit into the span
 */
int64_t G_random_layout_batches(const struct G_random_layout *layout)
{
    return layout->batches;
}

/*!
 * \brief Return the number of values a unit may draw
 *
 * \param[in] layout an initialized layout
 *
 * \return the stride of the layout, the number of values every unit may
 *         draw without running into the next unit's stream
 */
int64_t G_random_layout_length(const struct G_random_layout *layout)
{
    return layout->stride;
}

/*!
 * \brief Put a generator state at the start of a unit's stream in a batch
 *
 * A batch is one stream for every unit of the layout, and the batches
 * follow one another along the span. They start units * stride draws
 * apart, or one more when that number is even, and the stream of \p unit
 * in \p batch starts \p unit * stride draws after the start of the batch.
 * With the odd distance, as with an odd stride, the same draw of different
 * batches is as unrelated as the generator allows; what an even distance
 * would relate there is related between draws whose numbers differ
 * instead. The state is set whatever it held before, so the same call
 * always restarts the same sequence. See \ref gislib_random_streams.
 *
 * A \p unit outside 0 to units - 1, a negative \p batch, a \p batch other
 * than 0 of a layout of the whole span, or a \p batch beyond the batches
 * that fit is a fatal error. When no batch fits, batch 0 is still allowed,
 * and only batch 0: a tool which warned that its layout does not fit into
 * the span may use it as earlier versions did.
 *
 * Thread-safe as long as no two threads use the same state; the layout
 * is only read.
 *
 * \param[out] state generator state to set
 * \param[in] layout an initialized layout
 * \param[in] batch number of the batch, from 0
 * \param[in] unit number of the unit, from 0 to units - 1
 */
void G_random_state_for_batch(struct G_random_state *state,
                              const struct G_random_layout *layout,
                              int64_t batch, int64_t unit)
{
    uint64_t offset;

    if (unit < 0 || unit >= layout->units)
        G_fatal_error(_("Random number unit %" PRId64 " is out of range "
                        "(must be between 0 and %" PRId64 ")"),
                      unit, layout->units - 1);
    if (batch < 0)
        G_fatal_error(_("Random number batch %" PRId64 " is out of range "
                        "(must not be negative)"),
                      batch);
    if (batch > 0 && layout->whole_span)
        G_fatal_error(_("Random number batch %" PRId64 " is out of range "
                        "(a layout of the whole span holds a single batch)"),
                      batch);
    if (layout->batches >= 1 && batch >= layout->batches)
        G_fatal_error(n_("Random number batch %" PRId64 " is out of range "
                         "(%" PRId64 " batch fits into the generator's span)",
                         "Random number batch %" PRId64 " is out of range "
                         "(%" PRId64 " batches fit into the generator's span)",
                         (unsigned long)layout->batches),
                      batch, layout->batches);
    if (layout->batches == 0 && batch > 0)
        G_fatal_error(_("Random number batch %" PRId64 " is out of range "
                        "(no batch fits into the generator's span, only batch "
                        "0 can be used)"),
                      batch);

    /* With batch below the batches that fit, the offset is below 2^46; with
     * batch 0 of a layout which does not fit, it may reach past the span.
     * It is below 2^63 either way, since units times stride is. */
    offset = (uint64_t)batch * batch_distance(layout) +
             (uint64_t)unit * (uint64_t)layout->stride;
    state->state = lcg_jump(layout->start, offset);
}

/*!
 * \brief Put a generator state at the start of a unit's stream
 *
 * The same as G_random_state_for_batch() with batch 0.
 *
 * \param[out] state generator state to set
 * \param[in] layout an initialized layout
 * \param[in] unit number of the unit, from 0 to units - 1
 */
void G_random_state_for_unit(struct G_random_state *state,
                             const struct G_random_layout *layout, int64_t unit)
{
    G_random_state_for_batch(state, layout, 0, unit);
}

/*!
 * \brief Advance a generator of the program's own as if values had been drawn
 *
 * Moves the state by \p draws draws without taking them one at a time,
 * so that the next G_random_double() returns what the draw after those
 * would have returned.
 *
 * A negative \p draws is a fatal error.
 *
 * \param[in,out] state a seeded generator state
 * \param[in] draws number of values to skip, not negative
 */
void G_random_advance(struct G_random_state *state, int64_t draws)
{
    if (draws < 0)
        G_fatal_error(_("Cannot advance a random number generator by "
                        "%" PRId64 " draws (the number must not be negative)"),
                      draws);
    state->state = lcg_jump(state->state, (uint64_t)draws);
}

/*!
 * \brief Generate a floating-point value in the range [0,1) from a
 *        generator of the program's own
 *
 * Thread-safe as long as no two threads share a state. Unlike
 * G_drand48(), this needs no atomics and so behaves identically on every
 * build.
 *
 * \param[in,out] state generator state, set with
 *                G_random_state_from_seed(), G_random_state_for_unit() or
 *                G_random_state_for_batch()
 *
 * \return the generated value
 */
double G_random_double(struct G_random_state *state)
{
    state->state = lcg_step(state->state);
    /* The state is below 2^53, so the conversion to double is exact. */
    return (double)state->state / 281474976710656.0; /* 2^48 */
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
