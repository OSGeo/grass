/*!
  \file gis/put_window.c

  \brief GIS Library - Modify window (i.e. GRASS region)

  (C) 2001-2009 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.

  \author Original author CERL
*/

#include <stdlib.h>
#include <grass/gis.h>

/*!
 * \brief Write the database region
 *
 * Writes the database region file (WIND) in the user's current mapset
 * from region. 

 * <b>Warning:</b> Since this routine actually changes the database
 * region, it should only be called by modules which the user knows
 * will change the region. It is probably fair to say that only the
 * <tt>g.region</tt>.
 *
 * \param[in,out] window pointer to Cell_head
 *
 * \return 1 on success
 * \return -1 on error (no diagnostic message is printed)
 */
int G_put_window(const struct Cell_head *window)
{
    char *wind = getenv("WIND_OVERRIDE");

    return wind ? G__put_window(window, "windows", wind)
	: G__put_window(window, "", "WIND");
}

/*!
 * \brief Write the database region
 *
 * Writes the database region file (WIND) in the user's current mapset
 * from region. 

 * <b>Warning:</b> Since this routine actually changes the database
 * region, it should only be called by modules which the user knows
 * will change the region. It is probably fair to say that only the
 * <tt>g.region</tt>.
 *
 * \param[in,out] window pointer to Cell_head
 * \param dir directory name
 * \param name file name
 *
 * \return 1 on success
 * \return -1 on error (no diagnostic message is printed)
 */
int G__put_window(const struct Cell_head *window, const char *dir, const char *name)
{
    FILE *fd;

    if (!(fd = G_fopen_new(dir, name)))
	return -1;

    G__write_Cell_head3(fd, window, 0);
    fclose(fd);

    return 1;
}
