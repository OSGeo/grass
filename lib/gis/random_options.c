/*!
 * \file lib/gis/random_options.c
 *
 * \brief GIS Library - Random number seed from the option of a tool
 *
 * SPDX-FileCopyrightText: 2026 GRASS Development Team
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

/*!
 * \brief Read the random number seed from a seed option
 *
 * Call after G_parser() with an option which has an answer. The answer is
 * read with strtoll(); an answer which is not an integer, has anything
 * after the integer, or is outside the range from -2^31 to 2^32 - 1 is a
 * fatal error naming the option. Leading white space and a sign are
 * allowed. The parser itself checks an integer option only loosely, so
 * these checks are needed.
 *
 * The result is a seed accepted by G_random_state_from_seed(), the layout
 * functions and G_srand48(). The function does nothing beyond parsing and
 * checking: recording the seed, or generating one when the option is not
 * given, is up to the tool.
 *
 * \param seed the seed option, usually G_OPT_M_SEED, whatever its key
 *
 * \return the seed, from -2^31 to 2^32 - 1
 */
int64_t G_random_seed_from_option(const struct Option *seed)
{
    long long value;
    char *end;

    if (!seed->answer)
        G_fatal_error(_("%s= is required"), seed->key);
    errno = 0;
    value = strtoll(seed->answer, &end, 10);
    if (end == seed->answer || *end != '\0')
        G_fatal_error(_("Invalid random seed <%s> for %s=: not an integer"),
                      seed->answer, seed->key);
    /* The range is checked on the value strtoll() returns, so the
     * conversion to the return type is exact. */
    if (errno == ERANGE || value < -(INT64_C(1) << 31) ||
        value > (INT64_C(1) << 32) - 1)
        G_fatal_error(_("Invalid random seed <%s> for %s=: outside the range "
                        "from -2147483648 to 4294967295"),
                      seed->answer, seed->key);
    return value;
}
