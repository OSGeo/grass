/* random.c (simlib), 20.nov.2002, JH */

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include <grass/simlib.h>

/*!
 * \brief Return the seed of the walkers' random numbers
 *
 * The seed is generated with the flag, read from the option, or 12345 when
 * neither is given. A seed which is not an integer or is outside the range
 * the generator accepts is a fatal error, here, before any input is read.
 *
 * \param seed the seed option
 * \param generate the flag to generate a seed
 *
 * \return the seed
 */
int64_t simwe_seed(const struct Option *seed, const struct Flag *generate)
{
    int64_t value;
    struct G_random_state check;

    if (generate->answer) {
        value = G_random_generate_seed();
        G_verbose_message(_("Generated random seed (-s): %" PRId64), value);
    }
    else if (seed->answer) {
        char *end;

        errno = 0;
        value = strtoll(seed->answer, &end, 10);
        if (end == seed->answer || *end != '\0' || errno == ERANGE)
            G_fatal_error(_("Invalid random seed <%s>"), seed->answer);
        G_verbose_message(_("Read random seed from %s option: %" PRId64),
                          seed->key, value);
    }
    else {
        /* default as it used to be */
        value = 12345;
    }
    /* The layout, which refuses a seed out of range, is built only when the
     * number of time steps is known. */
    G_random_state_from_seed(&check, value);
    return value;
}

/*!
 * \brief Draw a pair of independent standard normal values
 *
 * Uses the polar method, which rejects points outside the unit circle, so
 * the number of values drawn varies, 8 / pi on average.
 *
 * \param state the random number state of the walker
 * \param[out] x the first value
 * \param[out] y the second value
 */
void gasdev(struct G_random_state *state, double *x, double *y)
{
    double r = 0.0, vv1 = 0.0, vv2 = 0.0, fac = 0.0;

    while (r >= 1. || r == 0.) {
        vv1 = G_random_double(state) * 2. - 1.;
        vv2 = G_random_double(state) * 2. - 1.;
        r = vv1 * vv1 + vv2 * vv2;
    }
    fac = sqrt(log(r) * -2. / r);
    (*y) = vv1 * fac;
    (*x) = vv2 * fac;
}
