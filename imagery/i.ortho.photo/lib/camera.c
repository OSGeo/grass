
/**********************************************************
* I_get_group_camera (group, &Cam_Ref);
* I_put_group_camera (group, &Cam_Ref);
**********************************************************/
#include "orthophoto.h"
#include <grass/ortholib.h>
#include <grass/glocale.h>

/* Put the "camera" name into the group file "CAMERA" */
int I_put_group_camera(char *group, char *camera)
{
    FILE *fd;

    G_suppress_warnings(1);
    fd = I_fopen_group_camera_new(group);
    G_suppress_warnings(0);
    if (!fd)
	return 0;

    fprintf(fd, "%s\n", camera);

    return 0;
}

/* Return the camera name from the group file CAMERA */
int I_get_group_camera(char *group, char *camera)
{
    char buf[200];
    FILE *fd;

    G_suppress_warnings(1);
    fd = I_fopen_group_camera_old(group);
    G_suppress_warnings(0);
    if (!fd) {
	sprintf(buf,
		_("Unable to open camera file for group <%s> in mapset <%s>"),
		group, G_mapset());
	G_warning(buf);
	return 0;
    }
    G_getl2(buf, sizeof(buf), fd);
    sscanf(buf, "%s", camera);
    return 1;
}
