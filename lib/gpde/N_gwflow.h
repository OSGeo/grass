
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      groundwater flow in porous media 
* 		part of the gpde library
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#ifndef _N_GWFLOW_H_
#define _N_GWFLOW_H_
#include "N_pde.h"
#include <math.h>

#define N_GW_CONFINED 0		/*confined groundwater */
#define N_GW_UNCONFINED 1	/*unconfined groundwater */

#define N_GW_DRY_CELL 0		/*a dry cell */
#define N_GW_SURFACE_CELL 1	/*a surface cell */
#define N_GW_NORMAL_CELL 2	/*a normal wet cell */

/*!
 * \brief This data structure contains all data needed to compute the 
 * groundwater mass balance in three dimension 
 * */
typedef struct
{
    N_array_3d *phead;		/*!piezometric head [m] */
    N_array_3d *phead_start;	/*!start conditions [m] */
    N_array_3d *hc_x;		/*!x part of the hydraulic conductivity tensor  [m/s] */
    N_array_3d *hc_y;		/*!y part of the hydraulic conductivity tensor  [m/s] */
    N_array_3d *hc_z;		/*!z part of the hydraulic conductivity tensor  [m/s] */
    N_array_3d *q;		/*!sources and sinks  [m^3/s] */
    N_array_2d *r;		/*!recharge at the top of the gw leayer  [1/s] */
    N_array_3d *s;		/*!specific yield [1/m] */
    N_array_3d *nf;		/*!effective porosity [-] */

    /*river */
    N_array_3d *river_leak;	/*!Leakage of the river bed [1/s] */
    N_array_3d *river_head;	/*!Waterlevel of the river [m] */
    N_array_3d *river_bed;	/*!Bed of the river [m] */

    /*drainage */
    N_array_3d *drain_leak;	/*!Leakage of the drainage bed [1/s] */
    N_array_3d *drain_bed;	/*!Bed of the drainage [m] */

    N_array_3d *status;		/*!active/inactive/dirichlet cell status */
    N_array_3d *drycells;	/*!array of dry cells */

    double dt;			/*!calculation time [s] */

} N_gwflow_data3d;

/*!
 * \brief This data structure contains all data needed to compute the 
 * groundwater mass balance in two dimension 
 * */
typedef struct
{
    N_array_2d *phead;		/*!piezometric head [m] */
    N_array_2d *phead_start;	/*!start conditions [m] */
    N_array_2d *hc_x;		/*!x part of the hydraulic conductivity tensor [m/s] */
    N_array_2d *hc_y;		/*!y part of the hydraulic conductivity tensor [m/s] */
    N_array_2d *q;		/*!sources and sinks [m^3/s] */
    N_array_2d *r;		/*!recharge at the top of the gw leayer [1/s] */
    N_array_2d *s;		/*!specific yield [1/m] */
    N_array_2d *nf;		/*!effective porosity [-] */

    /*river */
    N_array_2d *river_leak;	/*!Leakage of the river bed [1/s] */
    N_array_2d *river_head;	/*!Waterlevel of the river  [m] */
    N_array_2d *river_bed;	/*!Bed of the river [m] */

    /*drainage */
    N_array_2d *drain_leak;	/*!Leakage of the drainage bed [1/s] */
    N_array_2d *drain_bed;	/*!Bed of the drainage */


    N_array_2d *top;		/*!top surface of the quifer  [m] */
    N_array_2d *bottom;		/*!bottom of the aquifer  [m] */

    N_array_2d *status;		/*!active/inactive/dirichlet cell status */
    N_array_2d *drycells;	/*!array of dry cells */

    double dt;			/*!calculation time */
    int gwtype;			/*!Which type of groundwater, N_GW_CONFINED or N_GW_UNCONFIED */

} N_gwflow_data2d;

extern N_data_star *N_callback_gwflow_3d(void *gwdata, N_geom_data * geom,
					 int col, int row, int depth);
extern N_data_star *N_callback_gwflow_2d(void *gwdata, N_geom_data * geom,
					 int col, int row);
extern void N_gwflow_3d_calc_water_budget(N_gwflow_data3d * data,
        N_geom_data * geom, N_array_3d * budget);
extern void N_gwflow_2d_calc_water_budget(N_gwflow_data2d * data,
        N_geom_data * geom, N_array_2d * balance);
extern N_gwflow_data3d *N_alloc_gwflow_data3d(int cols, int rows, int depths,
					      int river, int drain);
extern N_gwflow_data2d *N_alloc_gwflow_data2d(int cols, int rows, int river,
					      int drain);
extern void N_free_gwflow_data3d(N_gwflow_data3d * data);
extern void N_free_gwflow_data2d(N_gwflow_data2d * data);
#endif
