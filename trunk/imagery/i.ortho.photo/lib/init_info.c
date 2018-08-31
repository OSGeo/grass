/* init_info.c */

#include "orthophoto.h"


FILE *I_fopen_group_init_old();
FILE *I_fopen_group_init_new();

#define INITIAL_FILE "INIT_EXP"

int I_read_init_info(FILE * fd, struct Ortho_Camera_Exp_Init *init_info)
{
    char buf[100];
    double XC, YC, ZC, omega, phi, kappa;
    double XCv, YCv, ZCv, omegav, phiv, kappav;
    int status;

    G_getl(buf, sizeof buf, fd);
    G_strip(buf);
    if (sscanf(buf, "INITIAL XC %lf \n", &XC) == 1)
	init_info->XC_init = XC;
    G_getl(buf, sizeof buf, fd);
    G_strip(buf);
    if (sscanf(buf, "INITIAL YC %lf \n", &YC) == 1)
	init_info->YC_init = YC;
    G_getl(buf, sizeof buf, fd);
    G_strip(buf);
    if (sscanf(buf, "INITIAL ZC %lf \n", &ZC) == 1)
	init_info->ZC_init = ZC;
    G_getl(buf, sizeof buf, fd);
    G_strip(buf);
    if (sscanf(buf, "INITIAL OMEGA %lf \n", &omega) == 1)
	init_info->omega_init = omega;
    G_getl(buf, sizeof buf, fd);
    G_strip(buf);
    if (sscanf(buf, "INITIAL PHI %lf \n", &phi) == 1)
	init_info->phi_init = phi;
    G_getl(buf, sizeof buf, fd);
    G_strip(buf);
    if (sscanf(buf, "INITIAL KAPPA %lf \n", &kappa) == 1)
	init_info->kappa_init = kappa;

    G_getl(buf, sizeof buf, fd);
    G_strip(buf);
    if (sscanf(buf, "VARIANCE XC %lf \n", &XCv) == 1)
	init_info->XC_var = XCv;
    G_getl(buf, sizeof buf, fd);
    G_strip(buf);
    if (sscanf(buf, "VARIANCE YC %lf \n", &YCv) == 1)
	init_info->YC_var = YCv;
    G_getl(buf, sizeof buf, fd);
    G_strip(buf);
    if (sscanf(buf, "VARIANCE ZC %lf \n", &ZCv) == 1)
	init_info->ZC_var = ZCv;
    G_getl(buf, sizeof buf, fd);
    G_strip(buf);
    if (sscanf(buf, "VARIANCE OMEGA %lf \n", &omegav) == 1)
	init_info->omega_var = omegav;
    G_getl(buf, sizeof buf, fd);
    G_strip(buf);
    if (sscanf(buf, "VARIANCE PHI %lf \n", &phiv) == 1)
	init_info->phi_var = phiv;
    G_getl(buf, sizeof buf, fd);
    G_strip(buf);
    if (sscanf(buf, "VARIANCE KAPPA %lf \n", &kappav) == 1)
	init_info->kappa_var = kappav;
    G_getl(buf, sizeof buf, fd);
    G_strip(buf);
    if (sscanf(buf, "STATUS (1=OK, 0=NOT OK) %d \n", &status) == 1)
	init_info->status = status;
    return 1;
}

int I_write_init_info(FILE * fd, struct Ortho_Camera_Exp_Init *init_info)
{
    fprintf(fd, "INITIAL XC    %f \n", init_info->XC_init);
    fprintf(fd, "INITIAL YC    %f \n", init_info->YC_init);
    fprintf(fd, "INITIAL ZC    %f \n", init_info->ZC_init);
    fprintf(fd, "INITIAL OMEGA %f \n", init_info->omega_init);
    fprintf(fd, "INITIAL PHI   %f \n", init_info->phi_init);
    fprintf(fd, "INITIAL KAPPA %f \n", init_info->kappa_init);

    fprintf(fd, "VARIANCE XC    %f \n", init_info->XC_var);
    fprintf(fd, "VARIANCE YC    %f \n", init_info->YC_var);
    fprintf(fd, "VARIANCE ZC    %f \n", init_info->ZC_var);
    fprintf(fd, "VARIANCE OMEGA %f \n", init_info->omega_var);
    fprintf(fd, "VARIANCE PHI   %f \n", init_info->phi_var);
    fprintf(fd, "VARIANCE KAPPA %f \n", init_info->kappa_var);
    fprintf(fd, "STATUS (1=OK, 0=NOT OK) %d \n", init_info->status);

    return 0;
}

int I_get_init_info(char *group, struct Ortho_Camera_Exp_Init *init_info)
{
    FILE *fd;
    char msg[100];
    int stat;

    fd = I_fopen_group_init_old(group);
    if (fd == NULL) {
	sprintf(msg, "unable to open camera initial file %s in %s",
		group, G_mapset());
	G_warning("%s", msg);
	return 0;
    }

    stat = I_read_init_info(fd, init_info);
    fclose(fd);
    if (stat < 0) {
	sprintf(msg, "bad format in camera initial file %s in %s",
		group, G_mapset());
	G_warning("%s", msg);
	return 0;
    }
    return 1;
}

int I_put_init_info(char *group, struct Ortho_Camera_Exp_Init *init_info)
{
    FILE *fd;
    char msg[100];

    fd = I_fopen_group_init_new(group);
    if (fd == NULL) {
	sprintf(msg, "unable to open camera initial file %s in %s",
		group, G_mapset());
	G_warning("%s", msg);
	return 0;
    }

    I_write_init_info(fd, init_info);
    fclose(fd);
    return 1;
}
