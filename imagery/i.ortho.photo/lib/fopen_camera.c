#include <grass/gis.h>

/******************************************************
* I_fopen_cam_file_new()
* I_fopen_cam_file_append()
* I_fopen_cam_file_old()
*
* fopen new camera files in the current mapset
* fopen old camera files anywhere
*******************************************************/
static int error(char *, char *, char *);

FILE *I_fopen_cam_file_new(char *camera)
{
    FILE *fd;

    fd = G_fopen_new("camera", camera);
    if (!fd)
	error(camera, "can't create ", "");
    return fd;
}

FILE *I_fopen_cam_file_append(char *camera)
{
    FILE *fd;

    fd = G_fopen_append("camera", camera);
    if (!fd)
	error(camera, "unable to open ", "");
    return fd;
}

FILE *I_fopen_cam_file_old(char *camera)
{
    FILE *fd;

    fd = G_fopen_old("camera", camera, G_mapset());
    if (!fd)
	error(camera, "can't open ", "");
    return fd;
}

static int error(char *camera, char *msga, char *msgb)
{
    char buf[100];

    sprintf(buf, "%s camera file [%s] in [%s %s] %s",
	    msga, camera, G_location(), G_mapset(), msgb);
    G_warning(buf);

    return 0;
}
