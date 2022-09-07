#ifndef GRASS_GLOCALE_H
#define GRASS_GLOCALE_H

#include <grass/config.h>

#include <grass/defs/glocale.h>

#if defined(HAVE_LIBINTL_H) && defined(USE_NLS)
#include <libintl.h>
#define _(str) G_gettext(PACKAGE,(str))
#define n_(strs,strp,num) G_ngettext(PACKAGE,(strs),(strp),num)
#else
#define _(str) (str)
#define n_(strs,strp,num) ((num == 1) ? (strs) : (strp))
#endif

#endif
