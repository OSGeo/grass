#include <stdio.h>
#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

/*!
 * \brief read target information
 *
 * Reads the target <b>project</b> and <b>subproject</b>
 * from the TARGET file for the specified group. Returns 1 if successful; 0
 * otherwise (and prints a diagnostic error). This routine is used by
 * <i>g.gui.gcp</i> and <i>i.rectify</i> and probably should not be used by
 * other programs.
 * <b>Note.</b> This routine does <b>not</b> validate the target information.
 *
 *  \param group
 *  \param project
 *  \param subproject
 *  \return int
 */

int I_get_target(const char *group, char *project, char *subproject)
{
    FILE *fd;
    int ok;

    *project = *subproject = 0;
    G_suppress_warnings(1);
    fd = I_fopen_group_file_old(group, "TARGET");
    G_suppress_warnings(0);
    if (fd == NULL)
	return 0;

    ok = (fscanf(fd, "%s %s", project, subproject) == 2);
    fclose(fd);
    if (!ok) {
	*project = *subproject = 0;
	G_warning(_("Unable to read target file for group [%s]"), group);
    }

    return ok;
}


/*!
 * \brief write target information
 *
 * Writes the target <b>project</b> and <b>subproject</b> to
 * the TARGET file for the specified <b>group.</b> Returns 1 if successful; 0
 * otherwise (but no error messages are printed).
 * This routine is used by <i>i.target</i> and probably should not be used by
 * other programs.
 * <b>Note.</b> This routine does <b>not</b> validate the target
 * information.
 *
 *  \param group
 *  \param project
 *  \param subproject
 *  \return int
 */

int I_put_target(const char *group, const char *project, const char *subproject)
{
    FILE *fd;

    fd = I_fopen_group_file_new(group, "TARGET");
    if (fd == NULL)
	return 0;

    fprintf(fd, "%s\n%s\n", project, subproject);
    fclose(fd);

    return 1;
}
