
/***********************************************************************
 * I_get_ref_points()
 * I_put_ref_points()
 **********************************************************************/
#include "orthophoto.h"
#include <grass/imagery.h>

#define REF_POINT_FILE "REF_POINTS"

int I_read_ref_points(FILE * fd, struct Ortho_Photo_Points *cp)
{
    char buf[100];
    double e1, e2, n1, n2;
    int status;

    cp->count = 0;

    /* read the reference point lines. format is:
       image_east image_north  photo_x photo_y  status(1=ok)
     */
    cp->e1 = NULL;
    cp->e2 = NULL;
    cp->n1 = NULL;
    cp->n2 = NULL;
    cp->status = NULL;

    /*fprintf (stderr, "Try to read one point \n"); */
    while (G_getl(buf, sizeof(buf), fd)) {
	G_strip(buf);
	if (*buf == '#' || *buf == 0)
	    continue;
	if (sscanf(buf, "%lf%lf%lf%lf%d", &e1, &n1, &e2, &n2, &status) == 5)
	    I_new_ref_point(cp, e1, n1, e2, n2, status);
	else
	    return -4;
    }

    return 1;
}

int
I_new_ref_point(struct Ortho_Photo_Points *cp, double e1, double n1,
		double e2, double n2, int status)
{
    int i;
    size_t size;

    /*fprintf (stderr, "Try to new_ref_point \n"); */
    if (status < 0)
	return 0;
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

int I_write_ref_points(FILE * fd, struct Ortho_Photo_Points *cp)
{
    int i;

    fprintf(fd, "# %7s %15s %15s %15s %9s status\n", "", "image", "", "photo",
	    "");
    fprintf(fd, "# %15s %15s %15s %15s  (1=ok)\n", "east", "north",
            "x", "y");
    fprintf(fd, "#\n");
    for (i = 0; i < cp->count; i++)
	if (cp->status[i] >= 0)
	    fprintf(fd, "  %15f %15f %15f %15f %d\n",
		    cp->e1[i], cp->n1[i],  
		    cp->e2[i], cp->n2[i],  
		    cp->status[i]);

    return 0;
}

int I_get_ref_points(char *groupname, struct Ortho_Photo_Points *cp)
{
    FILE *fd;
    char msg[100];
    int stat;

    /*fprintf (stderr, "Try to f_open_group_file_old \n"); */
    fd = I_fopen_group_file_old(groupname, REF_POINT_FILE);
    if (fd == NULL) {
	sprintf(msg,
		"unable to open reference point file for group [%s in %s]",
		groupname, G_mapset());
	G_warning("%s", msg);
	return 0;
    }

    /*fprintf (stderr, "Try to read_ref_points \n"); */
    stat = I_read_ref_points(fd, cp);
    fclose(fd);
    if (stat < 0) {
	sprintf(msg,
		"bad format in reference point file for group [%s in %s]",
		groupname, G_mapset());
	G_warning("%s", msg);
	return 0;
    }
    return 1;
}

int I_put_ref_points(char *groupname, struct Ortho_Photo_Points *cp)
{
    FILE *fd;
    char msg[100];

    fd = I_fopen_group_file_new(groupname, REF_POINT_FILE);
    if (fd == NULL) {
	sprintf(msg,
		"unable to create reference point file for group [%s in %s]",
		groupname, G_mapset());
	G_warning("%s", msg);
	return 0;
    }

    I_write_ref_points(fd, cp);
    fclose(fd);
    return 1;
}
