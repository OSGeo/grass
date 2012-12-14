#include <string.h>
#include <grass/glocale.h>
#include "orthophoto.h"
#include <grass/ortholib.h>

#define IN_BUF 100
#define CAMERA_FILE "CAMERA"


int I_read_cam_info(FILE * fd, struct Ortho_Camera_File_Ref *cam_info)
{
    int n;
    char buf[IN_BUF];
    char cam_name[30];
    char cam_id[30];
    double Xp, Yp, CFL;
    int num_fid;
    char fid_id[30];
    double Xf, Yf;

    G_getl2(buf, IN_BUF, fd);
    G_strip(buf);
    if (sscanf(buf, "CAMERA NAME   %[^\n]", cam_name) == 1)
	strcpy(cam_info->cam_name, cam_name);

    G_getl2(buf, IN_BUF, fd);
    G_strip(buf);
    if (sscanf(buf, "CAMERA ID     %[^\n]", cam_id) == 1)
	strcpy(cam_info->cam_id, cam_id);

    G_getl2(buf, IN_BUF, fd);
    G_strip(buf);
    if (sscanf(buf, "CAMERA XP     %lf \n", &Xp) == 1)
	cam_info->Xp = Xp;

    G_getl2(buf, IN_BUF, fd);
    G_strip(buf);
    if (sscanf(buf, "CAMERA YP     %lf \n", &Yp) == 1)
	cam_info->Yp = Yp;

    G_getl2(buf, IN_BUF, fd);
    G_strip(buf);
    if (sscanf(buf, "CAMERA CFL    %lf \n", &CFL) == 1)
	cam_info->CFL = CFL;

    G_getl2(buf, IN_BUF, fd);
    G_strip(buf);
    if (sscanf(buf, "NUM FID       %d \n", &num_fid) == 1)
	cam_info->num_fid = num_fid;

    for (n = 0; n < cam_info->num_fid; n++) {
	G_getl2(buf, IN_BUF, fd);
	G_strip(buf);
	if (sscanf(buf, "%s %lf %lf", fid_id, &Xf, &Yf) == 3) {
	    strcpy(cam_info->fiducials[n].fid_id, fid_id);
	    cam_info->fiducials[n].Xf = Xf;
	    cam_info->fiducials[n].Yf = Yf;
	}
    }

    return 1;
}


int I_new_fid_point(struct Ortho_Camera_File_Ref *cam_info,
		    char fid_id[30], double Xf, double Yf)
{
    return 0;
}


int I_write_cam_info(FILE * fd, struct Ortho_Camera_File_Ref *cam_info)
{
    int i;

    fprintf(fd, "CAMERA NAME   %s \n", cam_info->cam_name);
    fprintf(fd, "CAMERA ID     %s \n", cam_info->cam_id);
    fprintf(fd, "CAMERA XP     %f \n", cam_info->Xp);
    fprintf(fd, "CAMERA YP     %f \n", cam_info->Yp);
    fprintf(fd, "CAMERA CFL    %f \n", cam_info->CFL);
    fprintf(fd, "NUM FID       %d \n", cam_info->num_fid);

    for (i = 0; i < cam_info->num_fid; i++)
	fprintf(fd, "  %5s %15f %15f \n",
		cam_info->fiducials[i].fid_id,
		cam_info->fiducials[i].Xf, cam_info->fiducials[i].Yf);

    return 0;
}


int I_get_cam_info(char *camera, struct Ortho_Camera_File_Ref *cam_info)
{
    FILE *fd;
    int stat;

    fd = I_fopen_cam_file_old(camera);
    if (fd == NULL) {
	G_warning(_("Unable to open camera file '%s' in '%s'"),
		  camera, G_mapset());

	return 0;
    }

    stat = I_read_cam_info(fd, cam_info);
    fclose(fd);
    if (stat < 0) {
	G_warning(_("Bad format in camera file '%s' in '%s'"),
		  camera, G_mapset());

	return 0;
    }

    return 1;
}


int I_put_cam_info(char *camera, struct Ortho_Camera_File_Ref *cam_info)
{
    FILE *fd;

    fd = I_fopen_cam_file_new(camera);
    if (fd == NULL) {
	G_warning(_("Unable to open camera file '%s' in '%s'"),
		  camera, G_mapset());

	return 0;
    }

    I_write_cam_info(fd, cam_info);
    fclose(fd);

    return 1;
}
