#include <stdlib.h>
#include <string.h>

#include "proto.h"

#include <grass/glocale.h>

/* Returns translated version of a string.
   If global variable to output strings for translation is set it spits them out */
char *translate(const char *arg)
{
    static const char *domain;

    if (arg == NULL)
	return arg;

    if (strlen(arg) == 0)
        return NULL; /* unset */
    
    if (*arg && translate_output) {
	fputs(arg, stdout);
	fputs("\n", stdout);
    }

#if defined(HAVE_LIBINTL_H) && defined(USE_NLS)
    if (!domain) {
	domain = getenv("GRASS_TRANSLATION_DOMAIN");
	if (domain)
	    G_putenv("GRASS_TRANSLATION_DOMAIN", "grassmods");
	else
	    domain = PACKAGE;
    }

    return G_gettext(domain, arg);
#else
    return arg;
#endif
}
