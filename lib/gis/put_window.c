/*
 **********************************************************************
 *
 * G_put_window (window)
 *      write the current mapset window
 **********************************************************************
 *
 * G__put_window (window, dir, name)
 *      write the window 'name' in 'mapset'
 *      returns -1  error
 *               1  ok
 *********************************************************************/

#include <stdlib.h>
#include <grass/gis.h>

/*!
 * \brief write the database region
 *
 * Writes the database region file (WIND) in the user's current mapset
 * from <b>region.</b> Returns 1 if the region is written ok. Returns -1 if not
 * (no diagnostic message is printed).
 * <b>Warning.</b> Since this routine actually changes the database region, it
 * should only be called by modules which the user knows will change the region.
 * It is probably fair to say that under GRASS 3.0 only the <i>g.region</i>,
 * and <i>d.zoom</i> modules should call this routine.
 *
 *  \param region
 *  \return int
 */

int G_put_window(const struct Cell_head *window)
{
    char *wind = getenv("WIND_OVERRIDE");

    return wind ? G__put_window(window, "windows", wind)
	: G__put_window(window, "", "WIND");
}

int G__put_window(const struct Cell_head *window, char *dir, char *name)
{
    FILE *fd;

    if (!(fd = G_fopen_new(dir, name)))
	return -1;

    G__write_Cell_head3(fd, window, 0);
    fclose(fd);

    return 1;
}
