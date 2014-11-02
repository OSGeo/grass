#include <stdio.h>
#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

/*!
 * \brief read target information
 *
 * Reads the target <b>location</b> and <b>mapset</b>
 * from the TARGET file for the specified group. Returns 1 if successful; 0
 * otherwise (and prints a diagnostic error). This routine is used by
 * <i>g.gui.gcp</i> and <i>i.rectify</i> and probably should not be used by
 * other programs.
 * <b>Note.</b> This routine does <b>not</b> validate the target information.
 *
 *  \param group
 *  \param location
 *  \param mapset
 *  \return int
 */

int I_get_target(const char *group, char *location, char *mapset)
{
    FILE *fd;
    int ok;

    *location = *mapset = 0;
    G_suppress_warnings(1);
    fd = I_fopen_group_file_old(group, "TARGET");
    G_suppress_warnings(0);
    if (fd == NULL)
	return 0;

    ok = (fscanf(fd, "%s %s", location, mapset) == 2);
    fclose(fd);
    if (!ok) {
	*location = *mapset = 0;
	G_warning(_("Unable to read target file for group [%s]"), group);
    }

    return ok;
}


/*!
 * \brief write target information
 *
 * Writes the target <b>location</b> and <b>mapset</b> to
 * the TARGET file for the specified <b>group.</b> Returns 1 if successful; 0
 * otherwise (but no error messages are printed).
 * This routine is used by <i>i.target</i> and probably should not be used by
 * other programs.
 * <b>Note.</b> This routine does <b>not</b> validate the target
 * information.
 *
 *  \param group
 *  \param location
 *  \param mapset
 *  \return int
 */

int I_put_target(const char *group, const char *location, const char *mapset)
{
    FILE *fd;

    fd = I_fopen_group_file_new(group, "TARGET");
    if (fd == NULL)
	return 0;

    fprintf(fd, "%s\n%s\n", location, mapset);
    fclose(fd);

    return 1;
}
