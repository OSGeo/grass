
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      solute transport in porous media
* 		part of the gpde library
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include "N_pde.h"

#ifndef _N_SOLUTE_TRANSPORT_H_
#define _N_SOLUTE_TRANSPORT_H_


typedef struct
{
    N_array_3d *c;		/*concentration */
    N_array_3d *c_start;	/*concentration at start */
    N_array_3d *diff_x;		/*x part of the diffusion tensor */
    N_array_3d *diff_y;		/*y part of the diffusion tensor */
    N_array_3d *diff_z;		/*z part of the diffusion tensor */
    N_array_3d *nf;		/*effective porosity */
    N_array_3d *cs;		/*concentration sources and sinks */
    N_array_3d *q;		/*well sources and sinks */
    N_array_3d *R;		/*retardation */
    N_array_3d *cin;		/*concentration input from wells */

    N_gradient_field_3d *grad;	/*velocity field */

    N_array_3d *status;		/*active/inactive/dirichlet cell status */

    N_array_3d *disp_xx;	/*x part of the dispersivity tensor */
    N_array_3d *disp_yy;	/*x part of the dispersivity tensor */
    N_array_3d *disp_zz;	/*x part of the dispersivity tensor */
    N_array_3d *disp_xy;	/*xy part of the dispersivity tensor */
    N_array_3d *disp_xz;	/*xz part of the dispersivity tensor */
    N_array_3d *disp_yz;	/*yz part of the dispersivity tensor */

    double dt;			/*calculation time */
    double al, at;		/*dispersivity length longditudinal and transversal */
    int stab;			/*stabilization criteria */

} N_solute_transport_data3d;


typedef struct
{
    N_array_2d *c;		/*concentration */
    N_array_2d *c_start;	/*concentration at start */
    N_array_2d *diff_x;		/*x part of the diffusion tensor */
    N_array_2d *diff_y;		/*y part of the diffusion tensor */
    N_array_2d *nf;		/*effective porosity */
    N_array_2d *cs;		/*concentration sources and sinks */
    N_array_2d *q;		/*well sources and sinks */
    N_array_2d *R;		/*retardation */
    N_array_2d *cin;		/*concentration  */

    N_gradient_field_2d *grad;	/*velocity field */

    N_array_2d *status;		/*active/inactive/dirichlet cell status */
    N_array_2d *top;		/* top surface of the aquifer */
    N_array_2d *bottom;		/* bottom surface of the aquifer */

    N_array_2d *disp_xx;	/*x part of the dispersivity tensor */
    N_array_2d *disp_yy;	/*x part of the dispersivity tensor */
    N_array_2d *disp_xy;	/*xy part of the dispersivity tensor */

    double dt;			/*calculation time */
    double al, at;		/*dispersivity length longditudinal and transversal */
    int stab;			/*stabilization criteria */

} N_solute_transport_data2d;


extern N_data_star *N_callback_solute_transport_3d(void *solutedata,
						   N_geom_data * geom,
						   int col, int row,
						   int depth);
extern N_data_star *N_callback_solute_transport_2d(void *solutedata,
						   N_geom_data * geom,
						   int col, int row);
extern N_solute_transport_data3d *N_alloc_solute_transport_data3d(int cols,
								  int rows,
								  int depths);
extern N_solute_transport_data2d *N_alloc_solute_transport_data2d(int cols,
								  int rows);
extern void N_free_solute_transport_data3d(N_solute_transport_data3d * data);
extern void N_free_solute_transport_data2d(N_solute_transport_data2d * data);

/*compute the dispersivity tensor */
extern void N_calc_solute_transport_disptensor_2d(N_solute_transport_data2d *
						  data);
extern void N_calc_solute_transport_disptensor_3d(N_solute_transport_data3d *
						  data);
extern void N_calc_solute_transport_transmission_2d(N_solute_transport_data2d
						    * data);
extern void N_calc_solute_transport_transmission_3d(N_solute_transport_data3d
						    * data);
#endif
