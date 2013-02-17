#include <grass/gis.h>
#include <grass/imagery.h>
#include "mat.h"

/* #define DEBUG  1 */

#define INITIAL_X_VAR   500
#define INITIAL_Y_VAR   500
#define INITIAL_Z_VAR  1000
#define INITIAL_OMEGA_VAR   0.01
#define INITIAL_PHI_VAR     0.01
#define INITIAL_KAPPA_VAR   0.1

struct Ortho_Image_Group_Ref
{
    int nfiles;
    struct Ortho_Image_Group_Ref_Files
    {
	char name[GNAME_MAX];
	char mapset[GMAPSET_MAX];
    } *file;
    struct Ortho_Ref_Color
    {
	unsigned char *table;	/* color table for min-max values */
	unsigned char *index;	/* data translation index */
	unsigned char *buf;	/* data buffer for reading color file */
	int fd;			/* for image i/o */
	CELL min, max;		/* min,max CELL values */
	int n;			/* index into Ref_Files */
    } red, grn, blu;
};

struct Ortho_Camera_File_Ref
{
    char cam_name[30];
    char cam_id[30];
    double Xp;
    double Yp;
    double CFL;
    int num_fid;
    struct Fiducial
    {
	char fid_id[30];
	double Xf;
	double Yf;
    } fiducials[20];
};

struct Ortho_Photo_Points
{
    int count;
    double *e1;
    double *n1;
    double *e2;
    double *n2;
    double *z1;
    double *z2;
    int *status;
};

/* Ortho_Control_Points is identical to Ortho_Photo_Points
 * Why ? */
struct Ortho_Control_Points
{
    int count;
    double *e1;
    double *n1;
    double *z1;
    double *e2;
    double *n2;
    double *z2;
    int *status;
};

struct Ortho_Camera_Exp_Init
{
    double XC_init;
    double YC_init;
    double ZC_init;
    double omega_init;
    double phi_init;
    double kappa_init;
    double XC_var;
    double YC_var;
    double ZC_var;
    double omega_var;
    double phi_var;
    double kappa_var;
    int status;
};


struct Ortho_Image_Group
{
    char name[GNAME_MAX];
    /* Ortho_Image_Group_Ref is identical to Ortho_Group_Ref, and
       we assume this is so in the code.  If Ortho_Image_Group_Ref
       is ever different, then there will have to be a new set of
       I_get_group_ref() functions to fill it.
       struct Ortho_Image_Group_Ref    group_ref; */
    struct Ref group_ref;
    struct Ortho_Camera_File_Ref camera_ref;
    struct Ortho_Photo_Points photo_points;
    struct Ortho_Control_Points control_points;
    struct Ortho_Camera_Exp_Init camera_exp;
    int ref_equation_stat;
    int con_equation_stat;
    double E12[3], N12[3], E21[3], N21[3], Z12[3], Z21[3];
    double XC, YC, ZC, omega, phi, kappa;
    MATRIX M, MI;
};

/* conz_points.c */
int I_new_con_point(struct Ortho_Control_Points *,
		    double, double, double, double, double, double, int);
int I_get_con_points(char *, struct Ortho_Control_Points *);
int I_put_con_points(char *, struct Ortho_Control_Points *);
int I_convert_con_points(char *, struct Ortho_Control_Points *,
			 struct Ortho_Control_Points *, double[3], double[3]);
/* georef.c */
int I_compute_ref_equations(struct Ortho_Photo_Points *,
			    double *, double *, double *, double *);
/* orthoref.c */
int I_compute_ortho_equations(struct Ortho_Control_Points *,
			      struct Ortho_Camera_File_Ref *,
			      struct Ortho_Camera_Exp_Init *, double *,
			      double *, double *, double *, double *,
			      double *, MATRIX *, MATRIX *);
int I_ortho_ref(double, double, double, double *, double *, double *,
		struct Ortho_Camera_File_Ref *, double, double, double,
		MATRIX);
int I_inverse_ortho_ref(double, double, double, double *, double *, double *,
			struct Ortho_Camera_File_Ref *, double, double,
			double, MATRIX);
/* ref_points.c */
int I_new_ref_point(struct Ortho_Photo_Points *, double, double, double,
		    double, int);
int I_get_ref_points(char *, struct Ortho_Photo_Points *);
int I_put_ref_points(char *, struct Ortho_Photo_Points *);

/* cam_info.h */
int I_read_cam_info(FILE *, struct Ortho_Camera_File_Ref *);
int I_new_fid_point(struct Ortho_Camera_File_Ref *, char *, double, double);
int I_write_cam_info(FILE *, struct Ortho_Camera_File_Ref *);
int I_get_cam_info(char *, struct Ortho_Camera_File_Ref *);
int I_put_cam_info(char *, struct Ortho_Camera_File_Ref *);

/* init_info.c */
int I_read_init_info(FILE *, struct Ortho_Camera_Exp_Init *);
int I_write_init_info(FILE *, struct Ortho_Camera_Exp_Init *);
int I_get_init_info(char *, struct Ortho_Camera_Exp_Init *);
int I_put_init_info(char *, struct Ortho_Camera_Exp_Init *);


#include <grass/ortholib.h>
