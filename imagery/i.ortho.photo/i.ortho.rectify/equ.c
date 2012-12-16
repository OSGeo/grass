#include "global.h"

/* compute target to photo equation */
int Compute_ortho_equation(void)
{

    /* struct Ortho_Control_Points  temp_points; */
    double e0, e1, e2, n0, n1, n2, z1, z2;
    int status, i;
    struct Ortho_Control_Points temp_points;

    /* alloc and fill temp control points */
    temp_points.count = 0;
    temp_points.status = NULL;
    temp_points.e1 = NULL;
    temp_points.n1 = NULL;
    temp_points.z1 = NULL;
    temp_points.e2 = NULL;
    temp_points.n2 = NULL;
    temp_points.z2 = NULL;

    /* e0, n0, equal photo coordinates not image coords */
    for (i = 0; i < group.control_points.count; i++) {
	status = group.control_points.status[i];
	e1 = group.control_points.e1[i];
	n1 = group.control_points.n1[i];
	z1 = group.control_points.z1[i];
	e2 = group.control_points.e2[i];
	n2 = group.control_points.n2[i];
	z2 = group.control_points.z2[i];

	/* image to photo transformation */
	I_georef(e1, n1, &e0, &n0, group.E12, group.N12, 1);
	I_new_con_point(&temp_points, e0, n0, z1, e2, n2, z2, status);
    }


    group.con_equation_stat = I_compute_ortho_equations(&temp_points,
							&group.camera_ref,
							&group.camera_exp,
							&group.XC, &group.YC,
							&group.ZC,
							&group.omega,
							&group.phi,
							&group.kappa,
							&group.M,
							&group.MI);

    return 0;
}

/* compute photo to image equation */
int Compute_ref_equation(void)
{
    group.ref_equation_stat = I_compute_ref_equations(&group.photo_points,
						      group.E12, group.N12,
						      group.E21, group.N21);

    return 0;
}
