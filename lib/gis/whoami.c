/*!
 * \file lib/gis/whoami.c
 *
 * \brief GIS Library - Login name functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */
#include <unistd.h>
#include <stdlib.h>

#ifndef _WIN32
#include <pwd.h>
#endif

#include <grass/gis.h>

/*!
 * \brief Gets user's name.
 *
 * Returns a pointer to a string containing the user's login name.
 *
 * Tries getlogin() first, then goes to the password file.
 * However, some masscomp getlogin() fails in ucb universe
 * because the ttyname(0) rotuine fails in ucb universe.
 * So we check for this, too.
 *
 * \return pointer to string ("anonymous" by default)
 */
const char *G_whoami(void)
{
    static int initialized;
    static const char *name;

    if (G_is_initialized(&initialized))
        return name;

#ifdef _WIN32
    name = getenv("USERNAME");
#endif
    if (!name || !*name)
        name = getenv("LOGNAME");

    if (!name || !*name)
        name = getenv("USER");

#ifndef _WIN32
    if (!name || !*name) {
        struct passwd *p = getpwuid(getuid());

        if (p && p->pw_name && *p->pw_name)
            name = G_store(p->pw_name);
    }
#endif

    if (!name || !*name)
        name = "anonymous";

    G_initialize_done(&initialized);

    return name;
}
