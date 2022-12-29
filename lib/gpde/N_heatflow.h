
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      Calculation of heatflow
* 		part of the gpde library
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#ifndef _N_HEATFLOW_H_
#define _N_HEATFLOW_H_
#include "N_pde.h"

typedef struct
{
    N_array_3d *t;		/*temperature */
    N_array_3d *t_start;	/*temperature start conditions */
    N_array_3d *gamma_x;	/*x part of the gamma tensor */
    N_array_3d *gamma_y;	/*y part of the gamma tensor */
    N_array_3d *gamma_z;	/*z part of the gamma tensor */
    N_array_3d *q;		/*sources and sinks */
    N_array_3d *rho;		/*density */
    N_array_3d *c;		/*c */

    N_array_3d *status;		/*active/inactive/dirichlet cell status */

    double dt;			/*calculation time */

} N_heatflow_data3d;

typedef struct
{
    N_array_2d *t;		/*temperature */
    N_array_2d *t_start;	/*temperature start conditions */
    N_array_2d *gamma_x;	/*x part of the gamma tensor */
    N_array_2d *gamma_y;	/*y part of the gamma tensor */
    N_array_2d *q;		/*sources and sinks */
    N_array_2d *rho;		/*density */
    N_array_2d *c;		/*c */

    N_array_2d *status;		/*active/inactive/dirichlet cell status */

    double dt;			/*calculation time */

} N_heatflow_data2d;

extern N_data_star *N_callback_heatflow_3d(void *heatdata,
						 N_geom_data * geom,
						 int depth, int row, int col);
extern N_data_star *N_callback_heatflow_2d(void *heatdata,
						 N_geom_data * geom, int row,
						 int col);
extern N_heatflow_data3d *N_alloc_heatflow_data3d(int depths, int rows,
						  int cols);
extern N_heatflow_data2d *N_alloc_heatflow_data2d(int rows, int cols);

extern void N_free_heatflow_data3d(N_heatflow_data3d * data);

extern void N_free_heatflow_data2d(N_heatflow_data2d * data);
#endif
