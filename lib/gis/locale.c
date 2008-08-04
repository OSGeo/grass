
/**
 * \file locale.c
 *
 * \brief GIS Library - Functions to handle locale.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2004-2008
 */

#include <grass/config.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <grass/glocale.h>


#if defined(HAVE_LIBINTL_H) && defined(USE_NLS)
static char *locale_dir(void)
{
    static char localedir[4096];
    const char *gisbase;

    if (*localedir)
	return localedir;

    gisbase = getenv("GISBASE");
    if (!gisbase || !*gisbase)
	return "";

    strcpy(localedir, gisbase);
    strcat(localedir, "/locale");

    return localedir;
}
#endif


/**
 * \brief Gets localized text.
 *
 * \param[in] package
 * \param[in] msgid
 * \retval char * Pointer to string
 */

char *G_gettext(const char *package, const char *msgid)
{
#if defined(HAVE_LIBINTL_H) && defined(USE_NLS)
    static char now_bound[4096];
    static int initialized;

    if (!initialized) {
	setlocale(LC_CTYPE, "");
	setlocale(LC_MESSAGES, "");
	initialized = 1;
    }

    if (strcmp(now_bound, package) != 0) {
	strcpy(now_bound, package);
	bindtextdomain(package, locale_dir());
    }

    return dgettext(package, msgid);
#else
    return (char *)msgid;
#endif
}
