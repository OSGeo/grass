#ifndef GRASS_GLOCALE_H
#define GRASS_GLOCALE_H

#include <grass/config.h>

extern char *G_gettext(const char *, const char *);

#if defined(HAVE_LIBINTL_H) && defined(USE_NLS)
#include <libintl.h>
#define _(str) G_gettext(PACKAGE,(str))
#else
#define _(str) (str)
#endif

#endif
