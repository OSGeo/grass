/*!
  \file gis/get_window.c

  \brief GIS Library - Get window (i.e. GRASS region)

  (C) 2001-2009 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.

  \author Original author CERL
*/

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "../raster/G.h"

static struct state {
    int initialized;
    struct Cell_head dbwindow;
} state;

static struct state *st = &state;

/*!
 * \brief Read the database region
 *
 * Reads the database region as stored in the WIND file in the user's
 * current mapset into region.
 *
 * 3D values are set to defaults if not available in WIND file.
 * An error message is printed and exit() is called if there is a problem reading
 * the region.
 *
 * <b>Note:</b> GRASS applications that read or write raster maps
 * should not use this routine since its use implies that the active
 * module region will not be used. Programs that read or write raster
 * map data (or vector data) can query the active module region using
 * G_window_rows() and G_window_cols().
 *
 * \param window pointer to Cell_head
 */

void G_get_window(struct Cell_head *window)
{
    const char *regvar, *err;

    if (G_is_initialized(&st->initialized)) {
	*window = st->dbwindow;
	return;
    }

    /* Optionally read the region from environment variable */
    regvar = getenv("GRASS_REGION");

    if (regvar) {
	char **tokens = G_tokenize(regvar, ";");
	err = G__read_Cell_head_array(tokens, &st->dbwindow, 0);
	G_free_tokens(tokens);
    }
    else {
	char *wind = getenv("WIND_OVERRIDE");
	if (wind)
	    err = G__get_window(&st->dbwindow, "windows", wind, G_mapset());
	else
	    err = G__get_window(&st->dbwindow, "", "WIND", G_mapset());
    }

    if (err)
	G_fatal_error(_("Region for current mapset %s. "
			"Run \"g.region\" to fix the current region."), err);

    *window = st->dbwindow;

    if (!G__.window_set) {
	G__.window_set = 1;
	G__.window = st->dbwindow;
    }

    G_initialize_done(&st->initialized);
}

/*!
 * \brief Read the default region
 *
 * Reads the default region for the location into <i>region.</i> 3D
 * values are set to defaults if not available in WIND file.
 *
 * An error message is printed and exit() is called if there is a
 * problem reading the default region.
 *
 * \param[out] window pointer to Cell_head
 */

void G_get_default_window(struct Cell_head *window)
{
    const char *err = G__get_window(window, "", "DEFAULT_WIND", "PERMANENT");

    if (err)
	G_fatal_error(_("Default region %s"), err);
}

/*!
  \brief Get cwindow (region) of selected map layer
  
  \param window pointer to Cell_head
  \param element element name
  \param name map name
  \param mapset mapset name

  \return string on error
  \return NULL on success
*/
char *G__get_window(struct Cell_head *window,
		    const char *element, const char *name, const char *mapset)
{
    FILE *fp;
    char *err;

    G_zero(window, sizeof(struct Cell_head));

    /* Read from file */
    fp = G_fopen_old(element, name, mapset);
    if (!fp)
	return G_store(_("is not set"));

    err = G__read_Cell_head(fp, window, 0);
    fclose(fp);

    if (err) {
	char msg[1024];

	sprintf(msg, _("is invalid\n%s"), err);
	G_free(err);
	return G_store(msg);
    }

    return NULL;
}
