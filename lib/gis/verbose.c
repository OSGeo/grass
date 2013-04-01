/*!
 * \file lib/gis/verbose.c
 *
 * \brief GIS Library - Subroutines to manage verbosity level
 *
 * Note that verbosity can be controlled by GRASS_VERBOSE environment
 * variable.
 *
 * See relevant subroutines:
 *  - G_percent()
 *  - G_important_message()
 *  - G_message()
 *  - G_verbose_message()
 *  - G_warning()
 *  - G_fatal_error()
 *
 * (C) 2001-2008, 2012-2013 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Jachym Cepicky - jachym.cepicky at gmail.com
 */

#include <stdlib.h>
#include <grass/config.h>
#include <grass/gis.h>

/*! \brief Maximum verbosity level */
#define MAXLEVEL 3
/*! \brief Standard verbosity level */
#define STDLEVEL 2
/*! \brief Minumum verbosity level (quiet) */
#define MINLEVEL 0

static struct state {
    int initialized;
    int verbose;       /* current verbosity level */
} state;

static struct state *st = &state;

/*!
 * \brief Get current verbosity level.
 *
 * Currently, there are 5 levels of verbosity (see return codes)
 *
 * \return -1 - nothing will be printed (also errors and warnings will be also discarded)
 * \return  0 - nothing will be printed except of errors and warnings
 *              (G_fatal_error(), G_warning()). Triggered by <tt>--q</tt> or <tt>--quiet</tt> flag..
 * \return  1 - only progress information (G_percent()) and important messages (G_important_message()) will be printed
 * \return  2 - all messages (G_message() and G_important_message()) will be printed
 * \return  3 - also verbose messages (G_verbose_message()) will be printed. Triggered by <tt>--v</tt> or <tt>--verbose</tt> flag.
 */
int G_verbose(void)
{
    const char *verstr;         /* string for GRASS_VERBOSE content */

    if (G_is_initialized(&(st->initialized)))
        return st->verbose;

    /* verbose not defined -> get it from env. */
    verstr = getenv("GRASS_VERBOSE");
    st->verbose = verstr ? atoi(verstr) : STDLEVEL;

    G_initialize_done(&(st->initialized));

    return st->verbose;
}

/*!
 * \brief Get max verbosity level.
 *
 * \return max verbosity level
 */
int G_verbose_max(void)
{
    return MAXLEVEL;
}

/*!
 * \brief Get standard verbosity level.
 *
 * \return standard verbosity level
 */
int G_verbose_std(void)
{
    return STDLEVEL;
}

/*!
 * \brief Get min verbosity level.
 *
 * \return min verbosity level
 */
int G_verbose_min(void)
{
    return MINLEVEL;
}

/*!
 * \brief Set verbosity level.
 *
 * - -1 - nothing will be printed (also errors and warnings will be also discarded)
 * -  0 - nothing will be printed except of errors and warnings
 *              (G_fatal_error(), G_warning()). Triggered by <tt>--q</tt> or <tt>--quiet</tt> flag.
 * -  1 - only progress information (G_percent()) and important messages (G_important_message()) will be printed
 * -  2 - all messages (G_message() and G_important_message()) will be printed
 * -  3 - also verbose messages (G_verbose_message()) will be printed. Triggered by <tt>--v</tt> or <tt>--verbose</tt> flag.
 *
 * \param level new verbosity level (-1,0,1,2,3)
 *
 * \return 0 on invalid verbosity level (verbosity level untouched)
 * \return 1 on success
 */
int G_set_verbose(int level)
{
    if (level == -1 || (level >= MINLEVEL && level <= MAXLEVEL)) {
        st->verbose = level;
        return 1;
    }

    return 0;
}
