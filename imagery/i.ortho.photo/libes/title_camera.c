#include <grass/imagery.h>
#include <grass/ortholib.h>

int I_get_cam_title(char *camera, char *title, int n)
{
    FILE *fd;

    *title = 0;
    G_suppress_warnings(1);
    fd = I_fopen_cam_file_old(camera);
    G_suppress_warnings(0);
    if (fd != NULL) {
	G_getl(title, n, fd);
	fclose(fd);
    }
    return fd != NULL;
}

int I_put_camera_title(char *camera, char *title)
{
    FILE *fd;

    fd = I_fopen_cam_file_new(camera);
    if (fd != NULL) {
	fprintf(fd, "%s\n", title);
	fclose(fd);
    }
    return fd != NULL;
}
