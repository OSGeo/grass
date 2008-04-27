/**
 * \file verbose.c
 *
 * \brief Functions to check for GRASS_VERBOSE environment variable.
 *
 * see also:
 * G_percent()
 * G_message()
 * G_warning()
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Jachym Cepicky - jachym.cepicky at centrum cz
 *
 * \date 2006
 */

#include <stdlib.h>
#include <grass/config.h>

#define MAXLEVEL 3
#define STDLEVEL 2
#define MINLEVEL 0


static int verbose = -1; /* current verbosity level */


/**
 * \fn int G_verbose (void)
 *
 * \brief Get current verbosity level.
 *
 * Currently, there are 4 levels of verbosity.
 *
 * \return 0 - module should print nothing but errors and warnings
 *		(G_fatal_error, G_warning). Triggered by "--q" or "--quiet".
 * \return 1 - module will print progress information (G_percent)
 * \return 2 - module will print all messages (G_message)
 * \return 3 - module will be very verbose. Triggered by "--v" or "--verbose".
 */

int G_verbose (void)

{
    char *verstr; /* string for GRASS_VERBOSE content */

    /* verbose not defined -> get it from env. */ 
    if ( verbose < 0 ) {

        if ( (verstr = getenv ( "GRASS_VERBOSE" )) ) {
            if ((verbose = atoi ( verstr )))
                ;
        }
        else
            verbose = STDLEVEL;
    }
    return verbose;
}


/**
 * \fn int G_verbose_max (void)
 *
 * \brief Get max verbosity level.
 *
 * \return max verbosity level
 */

int G_verbose_max (void)
{
    return MAXLEVEL;
}


/**
 * \fn int G_verbose_std (void)
 *
 * \brief Get standard verbosity level.
 *
 * \return standard verbosity level
 */

int G_verbose_std (void)
{
    return STDLEVEL;
}


/**
 * \fn int G_verbose_min (void)
 *
 * \brief Get min verbosity level.
 *
 * \return min verbosity level
 */

int G_verbose_min (void)
{
    return MINLEVEL;
}

/**
 * \fn int G_set_verbose (int level)
 *
 * \brief Set verbosity level.
 *
 * \param level: new verbosity level
 *
 * \return 0 - failed (verbosity level untouched)
 * \return 1 - success
 */

int G_set_verbose (int level)
{
  if (level >= MINLEVEL && level <= MAXLEVEL)
  {
      verbose = level;
      return 1;
  }

  return 0;
}
