#include <grass/imagery.h>
#include <grass/glocale.h>

#define POINT_FILE "POINTS"

static int I_read_control_points(FILE * fd, struct Control_Points *cp)
{
    char buf[100];
    double e1, e2, n1, n2;
    int status;

    cp->count = 0;

    /* read the control point lines. format is:
       image_east image_north  target_east target_north  status
     */
    cp->e1 = NULL;
    cp->e2 = NULL;
    cp->n1 = NULL;
    cp->n2 = NULL;
    cp->status = NULL;

    while (G_getl2(buf, sizeof buf, fd)) {
	G_strip(buf);
	if (*buf == '#' || *buf == 0)
	    continue;
	if (sscanf(buf, "%lf%lf%lf%lf%d", &e1, &n1, &e2, &n2, &status) == 5)
	    I_new_control_point(cp, e1, n1, e2, n2, status);
	else
	    return -4;
    }

    return 1;
}


/*!
 * \brief add new control point
 *
 * Once the
 * control points have been read into the <b>cp</b> structure, this routine
 * adds new points to it. The new control point is given by <b>e1</b> (column)
 * and <b>n1</b> (row) on the image, and the <b>e2</b> (east) and <b>n2</b>
 * (north) for the target database. The value of <b>status</b> should be 1 if
 * the point is a valid point; 0 otherwise.\remarks{Use of this routine implies
 * that the point is probably good, so <b>status</b> should be set to 1.}
 *
 *  \param cp
 *  \param e1
 *  \param n1
 *  \param e2
 *  \param n2
 *  \param status
 *  \return int
 */

int I_new_control_point(struct Control_Points *cp,
			double e1, double n1, double e2, double n2,
			int status)
{
    int i;
    unsigned int size;

    if (status < 0)
	return 1;
    i = (cp->count)++;
    size = cp->count * sizeof(double);
    cp->e1 = (double *)G_realloc(cp->e1, size);
    cp->e2 = (double *)G_realloc(cp->e2, size);
    cp->n1 = (double *)G_realloc(cp->n1, size);
    cp->n2 = (double *)G_realloc(cp->n2, size);
    size = cp->count * sizeof(int);
    cp->status = (int *)G_realloc(cp->status, size);

    cp->e1[i] = e1;
    cp->e2[i] = e2;
    cp->n1[i] = n1;
    cp->n2[i] = n2;
    cp->status[i] = status;

    return 0;
}

static int I_write_control_points(FILE * fd, const struct Control_Points *cp)
{
    int i;

    fprintf(fd, "# %7s %15s %15s %15s %9s status\n", "", "image", "",
	    "target", "");
    fprintf(fd, "# %15s %15s %15s %15s   (1=ok)\n", "east", "north", "east",
	    "north");
    fprintf(fd, "#\n");
    for (i = 0; i < cp->count; i++)
	if (cp->status[i] >= 0)
	    fprintf(fd, "  %15f %15f %15f %15f %4d\n",
		    cp->e1[i], cp->n1[i], cp->e2[i], cp->n2[i],
		    cp->status[i]);

    return 0;
}


/*!
 * \brief read group control points
 *
 * Reads the control points from the POINTS file
 * for the <b>group</b> into the <b>cp</b> structure. Returns 1 if
 * successful; 0 otherwise (and prints a diagnostic error).
 * <b>Note.</b> An error message is printed if the POINTS file is invalid, or
 * does not exist.
 *
 *  \param group
 *  \param cp
 *  \return int
 */

int I_get_control_points(const char *group, struct Control_Points *cp)
{
    FILE *fd;
    int stat;

    fd = I_fopen_group_file_old(group, POINT_FILE);
    if (fd == NULL) {
	G_warning(_("Unable to open control point file for group [%s in %s]"),
		  group, G_mapset());
	return 0;
    }

    stat = I_read_control_points(fd, cp);
    fclose(fd);
    if (stat < 0) {
	G_warning(_("Bad format in control point file for group [%s in %s]"),
		  group, G_mapset());
	return 0;
    }
    return 1;
}


/*!
 * \brief write group control points
 *
 * Writes the control points from the
 * <b>cp</b> structure to the POINTS file for the specified group.
 * <b>Note.</b> Points in <b>cp</b> with a negative <i>status</i> are not
 * written to the POINTS file.
 *
 *  \param group
 *  \param cp
 *  \return int
 */

int I_put_control_points(const char *group, const struct Control_Points *cp)
{
    FILE *fd;

    fd = I_fopen_group_file_new(group, POINT_FILE);
    if (fd == NULL) {
	G_warning(_("Unable to create control point file for group [%s in %s]"),
		  group, G_mapset());
	return 0;
    }

    I_write_control_points(fd, cp);
    fclose(fd);
    return 1;
}
