/*!
   \file lib/gis/resource_dirs.c

   \brief GIS Library - Get paths to resource directories.

   \author Nicklas Larsson

   (c) 2025 by the GRASS Development Team

   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

static const char *get_g_env(const char *);

const char *G_colors_dir(void)
{
    return get_g_env("GRASS_COLORSDIR");
}

const char *G_etcbin_dir(void)
{
    return get_g_env("GRASS_ETCBINDIR");
}

const char *G_etc_dir(void)
{
    return get_g_env("GRASS_ETCDIR");
}

const char *G_fonts_dir(void)
{
    return get_g_env("GRASS_FONTSDIR");
}

const char *G_locale_dir(void)
{
    return get_g_env("GRASS_LOCALEDIR");
}

static const char *get_g_env(const char *env_var)
{
    const char *value = getenv(env_var);
    if (value)
        return value;

    G_fatal_error(_("Incomplete GRASS session: Variable '%s' not set"),
                  env_var);
}
