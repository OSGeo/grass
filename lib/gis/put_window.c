/*!
  \file lib/gis/put_window.c

  \brief GIS Library - Modify window (i.e. GRASS region)

  (C) 2001-2009 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.

  \author Original author CERL
*/

#include <stdlib.h>
#include <grass/gis.h>

#include "gis_local_proto.h"

/*!
 * \brief Writes the region (window)
 *
 * Writes the region file (WIND) in the user's current mapset
 * or when environmental variable \c WIND_OVERRIDE is set,
 * it writes the region to file specified by \c WIND_OVERRIDE variable.
 *
 * When \c WIND_OVERRIDE is set the current process and child processes
 * are affected.
 * Otherwise the whole GRASS session is affected.
 *
 * \warning When environmental variable \c WIND_OVERRIDE is not set,
 * this routine actually changes the region.
 * So in this case it should only be called by modules which the user knows
 * will change the region. It is probably fair to say that only the
 * \gmod{g.region} should call this routine unless \c WIND_OVERRIDE is set.
 *
 * This function does not adjust the \p window before setting the region
 * so you should call G_adjust_Cell_head() before calling this function.
 *
 * \param[in,out] window pointer to Cell_head
 *
 * \return 1 on success
 * \return -1 on error (no diagnostic message is printed)
 *
 * \sa G_get_window(), G_set_window(), python.core.use_temp_region()
 */
int G_put_window(const struct Cell_head *window)
{
    char *wind = getenv("WIND_OVERRIDE");

    return wind ? G_put_element_window(window, "windows", wind)
	: G_put_element_window(window, "", "WIND");
}

/*!
 * \brief Write the region
 *
 * Writes the region file (WIND) in the user's current mapset
 * from region. 

 * <b>Warning:</b> Since this routine actually changes the 
 * region, it should only be called by modules which the user knows
 * will change the region. It is probably fair to say that only the
 * <tt>g.region</tt> should call this routine.
 *
 * \param[in,out] window pointer to Cell_head
 * \param dir directory name
 * \param name file name
 *
 * \return 1 on success
 * \return -1 on error (no diagnostic message is printed)
 *
 * \sa G_put_window()
 */
int G_put_element_window(const struct Cell_head *window, const char *dir, const char *name)
{
    FILE *fd;

    if (!(fd = G_fopen_new(dir, name)))
	return -1;

    G__write_Cell_head3(fd, window, 0);
    fclose(fd);

    return 1;
}
