/*!
 * \file lib/gis/random_options.c
 *
 * \brief GIS Library - Random number seed from the option of a tool
 *
 * SPDX-FileCopyrightText: 2026 GRASS Development Team
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdint.h>

#include <grass/gis.h>
#include <grass/glocale.h>

/*!
 * \brief Read the random number seed from a seed option
 *
 * Call after G_parser() with an option which has an answer. The answer is
 * read with G_random_parse_seed(); an answer which is not a seed is a
 * fatal error naming the option. The parser itself checks an integer
 * option only loosely, so this check is needed.
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
    int64_t value;
    const char *problem;

    if (!seed->answer)
        G_fatal_error(_("%s= is required"), seed->key);
    problem = G_random_parse_seed(seed->answer, &value);
    if (problem)
        G_fatal_error(_("Invalid random seed <%s> for %s=: %s"), seed->answer,
                      seed->key, problem);
    return value;
}
