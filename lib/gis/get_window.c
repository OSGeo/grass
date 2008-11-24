/*
 *************************************************************************
 * G_get_window (window)
 *     struct Cell_head *window
 *
 *      read the current mapset window
 *      dies if error
 *
 *************************************************************************
 * G_get_default_window (window)
 *     struct Cell_head *window
 *
 *      read the default window for the location
 *      dies if error
 *
 *************************************************************************
 * char *
 * G__get_window (window, element, name, mapset)
 *      read the window 'name' in 'element' in 'mapset'
 *      returns NULL if ok, error message if not
 ************************************************************************/

#include <stdlib.h>
#include "G.h"
#include <grass/gis.h>
#include <grass/glocale.h>

static struct state {
    int initialized;
    struct Cell_head dbwindow;
} state;

static struct state *st = &state;

/*!
 * \brief read the database region
 *
 * Reads the database region as stored in the WIND file in the user's
 * current mapset <b>into region.</b>
 * 3D values are set to defaults if not available in WIND file.
 * An error message is printed and exit( ) is called if there is a problem reading
 * the region.
 * <b>Note.</b> GRASS applications that read or write raster maps should not
 * use this routine since its use implies that the active module region will not
 * be used. Programs that read or write raster map data (or vector data) can
 * query the active module region <i>using G_window_rows and
 * G_window_cols..</i>
 *
 *  \param region
 *  \return int
 */

void G_get_window(struct Cell_head *window)
{
    const char *regvar, *err;

    if (st->initialized) {
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
	G_fatal_error(_("region for current mapset %s\nrun \"g.region\""), err);

    st->initialized = 1;
    *window = st->dbwindow;

    if (!G__.window_set) {
	G__.window_set = 1;
	G__.window = st->dbwindow;
    }
}


/*!
 * \brief read the default region
 *
 * Reads the default region for the location into <b>region.</b>
 * 3D values are set to defaults if not available in WIND file.
 * An error message is printed and exit( ) is called if there is a problem
 * reading the default region.
 *
 *  \param region
 *  \return int
 */

void G_get_default_window(struct Cell_head *window)
{
    const char *err = G__get_window(window, "", "DEFAULT_WIND", "PERMANENT");

    if (err)
	G_fatal_error(_("default region %s"), err);
}

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
