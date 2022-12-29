#include <grass/imagery.h>
#include <grass/ortholib.h>
#include <grass/gis.h>

/******************************************************
* I_open_cam_file_new()
* I_open_cam_file_old()
*
* open new and old imagery group files in the current mapset
*******************************************************/
static int camera_error(char *, char *, char *, char *);

int I_open_cam_file_new(char *camera, char *file)
{
    int fd;
    char element[100];

    /* get group element name */
    sprintf(element, "camera");

    fd = G_open_new(element, camera);
    if (fd < 0)
	camera_error(camera, file, "can't create ", "");
    return fd;
}

int I_open_cam_file_old(char *camera, char *file)
{
    int fd;
    char element[100];

    /* find the file first */
    if (!I_find_camera_file(camera, file)) {
	camera_error(camera, file, "", " not found");
	return -1;
    }

    /* get group element name */
    sprintf(element, "camera/%s", camera);

    fd = G_open_old(element, camera, G_mapset());
    if (fd < 0)
	camera_error(camera, file, "can't open ", "");
    return fd;
}

static int camera_error(char *camera, char *file, char *msga, char *msgb)
{
    char buf[100];

    sprintf(buf, "%sfile [%s] of group [%s in %s]%s",
	    msga, file, camera, G_mapset(), msgb);
    G_warning("%s", buf);

    return 0;
}
