
/**
 * \file whoami.c
 *
 * \brief GIS Library - Login name functions.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <unistd.h>
#include <stdlib.h>

#ifndef __MINGW32__
#include <pwd.h>
#endif

#include <grass/gis.h>


/**
 * \brief Gets user's name.
 *
 * Returns a pointer to a string containing the user's login name.
 *
 * Tries getlogin() first, then goes to the password file.
 * However, some masscomp getlogin() fails in ucb universe
 * because the ttyname(0) rotuine fails in ucb universe.
 * So we check for this, too.
 *
 *  \retval char * Pointer to string
 */

char *G_whoami(void)
{
#ifdef __MINGW32__
    char *name = getenv("USERNAME");

    if (name == NULL) {
	name = "user_name";
    }
#else
    static char *name = NULL;

#ifdef COMMENTED_OUT
    char *getlogin();
    char *ttyname();

    if (name == NULL) {
	char *x;

	x = ttyname(0);
	if (x && *x) {
	    x = getlogin();
	    if (x && *x)
		name = G_store(x);
	}
    }
#endif /* COMMENTED_OUT */

    if (name == NULL) {
	struct passwd *p;

	if ((p = getpwuid(getuid())))
	    name = G_store(p->pw_name);
    }
    if (name == NULL)
	name = G_store("?");

#endif

    return name;
}
