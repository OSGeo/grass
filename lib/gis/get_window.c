/*!
   \file lib/gis/get_window.c

   \brief GIS Library - Get window (i.e. GRASS region)

   (C) 2001-2009, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL
 */

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "G.h"
#include "gis_local_proto.h"

static struct state {
    int initialized;
    struct Cell_head dbwindow;
} state;

static struct state *st = &state;

/*!
   \brief Get the current region

   Reads the region as stored in the WIND file in the user's current
   mapset into region.

   3D values are set to defaults if not available in WIND file.  An
   error message is printed and exit() is called if there is a problem
   reading the region.

   <b>Note:</b> GRASS applications that read or write raster maps
   should not use this routine since its use implies that the active
   module region will not be used. Programs that read or write raster
   map data (or vector data) can query the active module region using
   Rast_window_rows() and Rast_window_cols().

   \param[out] window pointer to Cell_head
 */
void G_get_window(struct Cell_head *window)
{
    const char *regvar;

    if (G_is_initialized(&st->initialized)) {
        *window = st->dbwindow;
        return;
    }

    /* Optionally read the region from environment variable */
    regvar = getenv("GRASS_REGION");

    if (regvar) {
        char **tokens = G_tokenize(regvar, ";");

        G__read_Cell_head_array(tokens, &st->dbwindow);
        G_free_tokens(tokens);
    }
    else {
        char *wind = getenv("WIND_OVERRIDE");

        if (wind) {
            char wind_env[GNAME_MAX] = {0};
            snprintf(wind_env, GNAME_MAX, "%s", wind);
            G_get_element_window(&st->dbwindow, "windows", wind_env,
                                 G_mapset());
        }
        else
            G_get_element_window(&st->dbwindow, "", "WIND", G_mapset());
    }

    *window = st->dbwindow;

    if (!G__.window_set) {
        G__.window_set = 1;
        G__.window = st->dbwindow;
    }

    G_initialize_done(&st->initialized);
}

/*!
   \brief Get the default region

   Reads the default region for the location into <i>region.</i> 3D
   values are set to defaults if not available in WIND file.

   An error message is printed and exit() is called if there is a
   problem reading the default region.

   \param[out] window pointer to Cell_head
 */
void G_get_default_window(struct Cell_head *window)
{
    G_get_element_window(window, "", "DEFAULT_WIND", "PERMANENT");
}

/*!
   \brief Get region for selected element (raster, vector, window, etc.)

   G_fatal_error() is called on error

   \param[out] window pointer to Cell_head
   \param element element type
   \param name element name
   \param mapset mapset name
 */
void G_get_element_window(struct Cell_head *window, const char *element,
                          const char *name, const char *mapset)
{
    FILE *fp;

    G_zero(window, sizeof(struct Cell_head));

    /* Read from file */
    fp = G_fopen_old(element, name, mapset);
    if (!fp)
        G_fatal_error(_("Unable to open element file <%s> for <%s@%s>"),
                      element, name, mapset);

    G_fseek(fp, 0, SEEK_END);
    if (!G_ftell(fp))
        G_fatal_error(_("Region file %s/%s/%s is empty"), mapset, element,
                      name);
    G_fseek(fp, 0, SEEK_SET);
    G__read_Cell_head(fp, window);
    fclose(fp);
}

/*!
   \brief Unset current region
 */
void G_unset_window(void)
{
    st->initialized = 0;
    G__.window_set = 0;
}
