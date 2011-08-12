
/**********************************************************************
 *
 *  Rast3d_read_history (name, mapset, hist)
 *      char *name                   name of map
 *      char *mapset                 mapset that map belongs to
 *      struct History *hist        structure to hold history info
 *
 *  Reads the history information associated with map layer "map"
 *  in mapset "mapset" into the structure "hist".
 *
 *   returns:    0  if successful
 *              -1  on fail
 *
 *  note:   a warning message is printed if the file is incorrect
 *
 **********************************************************************
 *
 *  Rast3d_write_history (name, hist)
 *      char *name                   name of map
 *      struct History *hist        structure holding history info
 *
 *  Writes the history information associated with map layer "map"
 *  into current from the structure "hist".
 *
 *   returns:    0  if successful
 *              -1  on fail
 **********************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <grass/glocale.h>
#include "raster3d_intern.h"
#include <grass/raster.h>

/*simple error message */
void SimpleErrorMessage(FILE * fd, const char *name, const char *mapset)
{
    if (fd != NULL)
	fclose(fd);

    G_warning(_("can't get history information for [%s] in mapset [%s]"),
	      name, mapset);
    return;
}

/*!
 * \brief read raster3d History file
 *
 * This routine reads the History file for
 * the raster3d file <b>name</b> in <b>mapset</b> into the <b>History</b>
 * structure.
 * A diagnostic message is printed and -1 is returned if there is an error
 * reading the History file. Otherwise, 0 is returned.
 * A warning message is printed if the file is incorrect.
 *
 *  \param name
 *  \param mapset
 *  \param history
 *  \return int
 */

int Rast3d_read_history(const char *name, const char *mapset, struct History *hist)
{
    FILE *fp;

    G_zero(hist, sizeof(struct History));

    fp = G_fopen_old_misc(RASTER3D_DIRECTORY, RASTER3D_HISTORY_ELEMENT, name, mapset);
    if (!fp)
	return -2;

    if (Rast__read_history(hist, fp) == 0)
	return 0;

    SimpleErrorMessage(fp, name, mapset);
    return -1;
}


/*!
 * \brief write raster3d History file
 *
 * This routine writes the History file for the raster3d file
 * <b>name</b> in the current mapset from the <b>History</b> structure.
 * A diagnostic message is printed and -1 is returned if there is an error
 * writing the History file. Otherwise, 0 is returned.
 * <b>Note.</b> The <b>history</b> structure should first be initialized
 * using <i>Rast_short_history.</i>
 *
 *  \param name
 *  \param history
 *  \return int
 */

int Rast3d_write_history(const char *name, struct History *hist)
/* This function is adapted from Rast_write_history */
{
    FILE *fp = G_fopen_new_misc(RASTER3D_DIRECTORY, RASTER3D_HISTORY_ELEMENT, name);
    if (!fp)
	return -1;

    Rast__write_history(hist, fp);
    return 0;
}
