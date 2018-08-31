
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

#include <grass/N_gwflow.h>

/* *************************************************************** */
/* ***************** N_gwflow_data3d ***************************** */
/* *************************************************************** */
/*!
 * \brief Alllocate memory for the groundwater calculation data structure in 3 dimensions
 *
 * The groundwater calculation data structure will be allocated including
 * all appendant 3d and 2d arrays. The offset for the 3d arrays is one
 * to establish homogeneous Neumann boundary conditions at the calculation area border.
 * This data structure is used to create a linear equation system based on the computation of
 * groundwater flow in porous media with the finite volume method.
 *
 * \param cols   int
 * \param rows   int
 * \param depths int
 * \return N_gwflow_data3d *
 * */
N_gwflow_data3d *N_alloc_gwflow_data3d(int cols, int rows, int depths,
				       int river, int drain)
{
    N_gwflow_data3d *data;

    data = (N_gwflow_data3d *) G_calloc(1, sizeof(N_gwflow_data3d));

    data->phead = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->phead_start = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->status = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->hc_x = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->hc_y = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->hc_z = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->q = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->s = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->nf = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->r = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);

    if (river) {
	data->river_head =
	    N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
	data->river_leak =
	    N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
	data->river_bed = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    }
    else {
	data->river_head = NULL;
	data->river_leak = NULL;
	data->river_bed = NULL;
    }

    if (drain) {
	data->drain_leak =
	    N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
	data->drain_bed = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    }
    else {
	data->drain_leak = NULL;
	data->drain_bed = NULL;
    }

    return data;
}

/* *************************************************************** */
/* ********************* N_free_gwflow_data3d ******************** */
/* *************************************************************** */
/*!
 * \brief Release the memory of the groundwater flow data structure in three dimensions
 *
 * \param data N_gwflow_data3d *
 * \return void *
 * */

void N_free_gwflow_data3d(N_gwflow_data3d * data)
{
    if (data->phead)
	N_free_array_3d(data->phead);
    if (data->phead_start)
	N_free_array_3d(data->phead_start);
    if (data->status)
	N_free_array_3d(data->status);
    if (data->hc_x)
	N_free_array_3d(data->hc_x);
    if (data->hc_y)
	N_free_array_3d(data->hc_y);
    if (data->hc_z)
	N_free_array_3d(data->hc_z);
    if (data->q)
	N_free_array_3d(data->q);
    if (data->s)
	N_free_array_3d(data->s);
    if (data->nf)
	N_free_array_3d(data->nf);
    if (data->r)
	N_free_array_2d(data->r);
    if (data->river_head)
	N_free_array_3d(data->river_head);
    if (data->river_leak)
	N_free_array_3d(data->river_leak);
    if (data->river_bed)
	N_free_array_3d(data->river_bed);
    if (data->drain_leak)
	N_free_array_3d(data->drain_leak);
    if (data->drain_bed)
	N_free_array_3d(data->drain_bed);

    G_free(data);

    data = NULL;

    return;
}

/* *************************************************************** */
/* ******************** N_alloc_gwflow_data2d ******************** */
/* *************************************************************** */
/*!
 * \brief Alllocate memory for the groundwater calculation data structure in 2 dimensions
 * 
 * The groundwater calculation data structure will be allocated including
 * all appendant 2d arrays. The offset for the 3d arrays is one
 * to establish homogeneous Neumann boundary conditions at the calculation area border.
 * This data structure is used to create a linear equation system based on the computation of
 * groundwater flow in porous media with the finite volume method.
 *
 * \param cols int
 * \param rows int
 * \return N_gwflow_data2d *
 * */
N_gwflow_data2d *N_alloc_gwflow_data2d(int cols, int rows, int river,
				       int drain)
{
    N_gwflow_data2d *data;

    data = (N_gwflow_data2d *) G_calloc(1, sizeof(N_gwflow_data2d));

    data->phead = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->phead_start = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->status = N_alloc_array_2d(cols, rows, 1, CELL_TYPE);
    data->hc_x = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->hc_y = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->q = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->s = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->nf = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->r = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->top = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->bottom = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);

    if (river) {
	data->river_head = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
	data->river_leak = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
	data->river_bed = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    }
    else {
	data->river_head = NULL;
	data->river_leak = NULL;
	data->river_bed = NULL;
    }

    if (drain) {
	data->drain_leak = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
	data->drain_bed = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    }
    else {
	data->drain_leak = NULL;
	data->drain_bed = NULL;
    }


    return data;
}

/* *************************************************************** */
/* ****************** N_free_gwflow_data2d *********************** */
/* *************************************************************** */
/*!
 * \brief Release the memory of the groundwater flow data structure in two dimensions
 *
 * \param data N_gwflow_data2d *
 * \return void
 * */
void N_free_gwflow_data2d(N_gwflow_data2d * data)
{
    if (data->phead)
	N_free_array_2d(data->phead);
    if (data->phead_start)
	N_free_array_2d(data->phead_start);
    if (data->status)
	N_free_array_2d(data->status);
    if (data->hc_x)
	N_free_array_2d(data->hc_x);
    if (data->hc_y)
	N_free_array_2d(data->hc_y);
    if (data->q)
	N_free_array_2d(data->q);
    if (data->s)
	N_free_array_2d(data->s);
    if (data->nf)
	N_free_array_2d(data->nf);
    if (data->r)
	N_free_array_2d(data->r);
    if (data->top)
	N_free_array_2d(data->top);
    if (data->bottom)
	N_free_array_2d(data->bottom);
    if (data->river_head)
	N_free_array_2d(data->river_head);
    if (data->river_leak)
	N_free_array_2d(data->river_leak);
    if (data->river_bed)
	N_free_array_2d(data->river_bed);
    if (data->drain_leak)
	N_free_array_2d(data->drain_leak);
    if (data->drain_bed)
	N_free_array_2d(data->drain_bed);

    G_free(data);

    data = NULL;;

    return;
}

/* *************************************************************** */
/* ***************** N_callback_gwflow_3d ************************ */
/* *************************************************************** */
/*!
 * \brief This callback function creates the mass balance of a 7 point star
 *
 * The mass balance is based on the common groundwater flow equation:
 *
 * \f[Ss \frac{\partial h}{\partial t} = \nabla {\bf K} \nabla h + q \f]
 *
 * This equation is discretizised with the finite volume method in three dimensions.
 *
 *
 * \param gwdata N_gwflow_data3d *
 * \param geom N_geom_data *
 * \param col   int
 * \param row   int
 * \param depth int
 * \return N_data_star *
 *
 * */
N_data_star *N_callback_gwflow_3d(void *gwdata, N_geom_data * geom, int col,
				  int row, int depth)
{
    double hc_e = 0, hc_w = 0, hc_n = 0, hc_s = 0, hc_t = 0, hc_b = 0;
    double dx, dy, dz, Ax, Ay, Az;
    double hc_x, hc_y, hc_z;
    double hc_xw, hc_yn, hc_zt;
    double hc_xe, hc_ys, hc_zb;
    double hc_start;
    double Ss, r, nf, q;
    double C, W, E, N, S, T, B, V;
    N_data_star *mat_pos;
    N_gwflow_data3d *data;

    /*cast the void pointer to the right data structure */
    data = (N_gwflow_data3d *) gwdata;

    dx = geom->dx;
    dy = geom->dy;
    dz = geom->dz;
    Az = N_get_geom_data_area_of_cell(geom, row);
    Ay = geom->dx * geom->dz;
    Ax = geom->dz * geom->dy;

    /*read the data from the arrays */
    hc_start = N_get_array_3d_d_value(data->phead_start, col, row, depth);

    hc_x = N_get_array_3d_d_value(data->hc_x, col, row, depth);
    hc_y = N_get_array_3d_d_value(data->hc_y, col, row, depth);
    hc_z = N_get_array_3d_d_value(data->hc_z, col, row, depth);

    hc_xw = N_get_array_3d_d_value(data->hc_x, col - 1, row, depth);
    hc_xe = N_get_array_3d_d_value(data->hc_x, col + 1, row, depth);
    hc_yn = N_get_array_3d_d_value(data->hc_y, col, row - 1, depth);
    hc_ys = N_get_array_3d_d_value(data->hc_y, col, row + 1, depth);
    hc_zt = N_get_array_3d_d_value(data->hc_z, col, row, depth + 1);
    hc_zb = N_get_array_3d_d_value(data->hc_z, col, row, depth - 1);

    hc_w = N_calc_harmonic_mean(hc_xw, hc_x);
    hc_e = N_calc_harmonic_mean(hc_xe, hc_x);
    hc_n = N_calc_harmonic_mean(hc_yn, hc_y);
    hc_s = N_calc_harmonic_mean(hc_ys, hc_y);
    hc_t = N_calc_harmonic_mean(hc_zt, hc_z);
    hc_b = N_calc_harmonic_mean(hc_zb, hc_z);

    /*inner sources */
    q = N_get_array_3d_d_value(data->q, col, row, depth);
    /*storativity */
    Ss = N_get_array_3d_d_value(data->s, col, row, depth);
    /*porosity */
    nf = N_get_array_3d_d_value(data->nf, col, row, depth);

    /*mass balance center cell to western cell */
    W = -1 * Ax * hc_w / dx;
    /*mass balance center cell to eastern cell */
    E = -1 * Ax * hc_e / dx;
    /*mass balance center cell to northern cell */
    N = -1 * Ay * hc_n / dy;
    /*mass balance center cell to southern cell */
    S = -1 * Ay * hc_s / dy;
    /*mass balance center cell to top cell */
    T = -1 * Az * hc_t / dz;
    /*mass balance center cell to bottom cell */
    B = -1 * Az * hc_b / dz;

    /*storativity */
    Ss = Az * dz * Ss;

    /*the diagonal entry of the matrix */
    C = -1 * (W + E + N + S + T + B - Ss / data->dt * Az);

    /*the entry in the right side b of Ax = b */
    V = (q + hc_start * Ss / data->dt * Az);

    /*only the top cells will have recharge */
    if (depth == geom->depths - 2) {
	r = N_get_array_2d_d_value(data->r, col, row);
	V += r * Az;
    }

    G_debug(5, "N_callback_gwflow_3d: called [%i][%i][%i]", depth, col, row);

    /*create the 7 point star entries */
    mat_pos = N_create_7star(C, W, E, N, S, T, B, V);

    return mat_pos;
}


/* *************************************************************** */
/* ****************** N_gwflow_3d_calc_water_budget ************** */
/* *************************************************************** */
/*!
 * \brief This function computes the water budget of the entire groundwater
 *
 * The water budget is calculated for each active and dirichlet cell from
 * its surrounding neighbours. This is based on the 7 star mass balance computation
 * of N_callback_gwflow_3d and the gradient of the water heights in the cells.
 * The sum of the water budget of each active/dirichlet cell must be near zero
 * due the effect of numerical inaccuracy of cpu's.
 *
 * \param gwdata N_gwflow_data3d *
 * \param geom N_geom_data *
 * \param budget N_array_3d
 * \return void
 *
 * */
void
N_gwflow_3d_calc_water_budget(N_gwflow_data3d * data, N_geom_data * geom, N_array_3d * budget)
{
    int z, y, x, stat;
    double h, hc;
    double val;
    double sum;
    N_data_star *dstar;

    int rows = data->status->rows;
    int cols = data->status->cols;
    int depths = data->status->depths;
    sum = 0;

    for (z = 0; z < depths; z++) {
        for (y = 0; y < rows; y++) {
            G_percent(y, rows - 1, 10);
            for (x = 0; x < cols; x++) {
                stat = (int)N_get_array_3d_d_value(data->status, x, y, z);

                val = 0.0;

                if (stat != N_CELL_INACTIVE ) {	/*all active/dirichlet cells */

                    /* Compute the flow parameter */
                    dstar = N_callback_gwflow_3d(data, geom, x, y, z);
                    /* Compute the gradient in each direction pointing from the center */
                    hc = N_get_array_3d_d_value(data->phead, x, y, z);

                    if((int)N_get_array_3d_d_value(data->status, x + 1, y    , z) != N_CELL_INACTIVE) {
                        h = N_get_array_3d_d_value(data->phead,  x + 1, y    , z);
                        val += dstar->E * (hc - h);
                    }
                    if((int)N_get_array_3d_d_value(data->status, x - 1, y    , z) != N_CELL_INACTIVE) {
                        h = N_get_array_3d_d_value(data->phead,  x - 1, y    , z);
                        val += dstar->W * (hc - h);
                    }
                    if((int)N_get_array_3d_d_value(data->status, x    , y + 1, z) != N_CELL_INACTIVE) {
                        h = N_get_array_3d_d_value(data->phead,  x    , y + 1, z);
                        val += dstar->S * (hc - h);
                    }
                    if((int)N_get_array_3d_d_value(data->status, x    , y - 1, z) != N_CELL_INACTIVE) {
                        h = N_get_array_3d_d_value(data->phead,  x    , y - 1, z);
                        val += dstar->N * (hc - h);
                    }
                    if((int)N_get_array_3d_d_value(data->status, x    , y    , z + 1) != N_CELL_INACTIVE) {
                        h = N_get_array_3d_d_value(data->phead,  x    , y    , z + 1);
                        val += dstar->T * (hc - h);
                    }
                    if((int)N_get_array_3d_d_value(data->status, x    , y    , z - 1) != N_CELL_INACTIVE) {
                        h = N_get_array_3d_d_value(data->phead,  x    , y    , z - 1);
                        val += dstar->B * (hc - h);
                    }
                    sum += val;

                    G_free(dstar);
                }
                else {
                    Rast_set_null_value(&val, 1, DCELL_TYPE);
                }
                N_put_array_3d_d_value(budget, x, y, z, val);
            }
        }
    }

    if(fabs(sum) < 0.0000000001)
        G_message(_("The total sum of the water budget: %g\n"), sum);
    else
        G_warning(_("The total sum of the water budget is significantly larger then 0: %g\n"), sum);

    return;
}



/* *************************************************************** */
/* ****************** N_callback_gwflow_2d *********************** */
/* *************************************************************** */
/*!
 * \brief This callback function creates the mass balance of a 5 point star
 *
 * The mass balance is based on the common groundwater flow equation:
 *
 * \f[Ss \frac{\partial h}{\partial t} = \nabla {\bf K} \nabla h + q \f]
 *
 * This equation is discretizised with the finite volume method in two dimensions.
 *
 * \param gwdata N_gwflow_data2d *
 * \param geom N_geom_data *
 * \param col int
 * \param row int
 * \return N_data_star *
 *
 * */
N_data_star *N_callback_gwflow_2d(void *gwdata, N_geom_data * geom, int col,
				  int row)
{
    double T_e = 0, T_w = 0, T_n = 0, T_s = 0;
    double z_e = 0, z_w = 0, z_n = 0, z_s = 0;
    double dx, dy, Az;
    double hc_x, hc_y;
    double z, top;
    double hc_xw, hc_yn;
    double z_xw, z_yn;
    double hc_xe, hc_ys;
    double z_xe, z_ys;
    double hc, hc_start;
    double Ss, r, q;
    double C, W, E, N, S, V;
    N_gwflow_data2d *data;
    N_data_star *mat_pos;
    double river_vect = 0;	/*entry in vector */
    double river_mat = 0;	/*entry in matrix */
    double drain_vect = 0;	/*entry in vector */
    double drain_mat = 0;	/*entry in matrix */

    /*cast the void pointer to the right data structure */
    data = (N_gwflow_data2d *) gwdata;

    dx = geom->dx;
    dy = geom->dy;
    Az = N_get_geom_data_area_of_cell(geom, row);

    /*read the data from the arrays */
    hc_start = N_get_array_2d_d_value(data->phead_start, col, row);
    hc = N_get_array_2d_d_value(data->phead, col, row);
    top = N_get_array_2d_d_value(data->top, col, row);

    /* Inner sources */
    q = N_get_array_2d_d_value(data->q, col, row);

    /* storativity or porosity of current cell face [-]*/
    Ss = N_get_array_2d_d_value(data->s, col, row);
    /* recharge */
    r = N_get_array_2d_d_value(data->r, col, row) * Az;


    if (hc > top) {		/*If the aquifer is confined */
	z = N_get_array_2d_d_value(data->top, col,
				   row) -
	    N_get_array_2d_d_value(data->bottom, col, row);
	z_xw =
	    N_get_array_2d_d_value(data->top, col - 1,
				   row) -
	    N_get_array_2d_d_value(data->bottom, col - 1, row);
	z_xe =
	    N_get_array_2d_d_value(data->top, col + 1,
				   row) -
	    N_get_array_2d_d_value(data->bottom, col + 1, row);
	z_yn =
	    N_get_array_2d_d_value(data->top, col,
				   row - 1) -
	    N_get_array_2d_d_value(data->bottom, col, row - 1);
	z_ys =
	    N_get_array_2d_d_value(data->top, col,
				   row + 1) -
	    N_get_array_2d_d_value(data->bottom, col, row + 1);
    }
    else {			/* the aquifer is unconfined */

	/* If the aquifer is unconfied use an explicite scheme to solve
	 * the nonlinear equation. We use the phead from the first iteration */
	z = N_get_array_2d_d_value(data->phead, col, row) -
	    N_get_array_2d_d_value(data->bottom, col, row);
	z_xw = N_get_array_2d_d_value(data->phead, col - 1, row) -
	    N_get_array_2d_d_value(data->bottom, col - 1, row);
	z_xe = N_get_array_2d_d_value(data->phead, col + 1, row) -
	    N_get_array_2d_d_value(data->bottom, col + 1, row);
	z_yn = N_get_array_2d_d_value(data->phead, col, row - 1) -
	    N_get_array_2d_d_value(data->bottom, col, row - 1);
	z_ys = N_get_array_2d_d_value(data->phead, col, row + 1) -
	    N_get_array_2d_d_value(data->bottom, col, row + 1);
    }

    /*geometrical mean of cell height */
    if (z_w > 0 || z_w < 0 || z_w == 0)
	z_w = N_calc_arith_mean(z_xw, z);
    else
	z_w = z;
    if (z_e > 0 || z_e < 0 || z_e == 0)
	z_e = N_calc_arith_mean(z_xe, z);
    else
	z_e = z;
    if (z_n > 0 || z_n < 0 || z_n == 0)
	z_n = N_calc_arith_mean(z_yn, z);
    else
	z_n = z;
    if (z_s > 0 || z_s < 0 || z_s == 0)
	z_s = N_calc_arith_mean(z_ys, z);
    else
	z_s = z;

    /*get the surrounding permeabilities */
    hc_x = N_get_array_2d_d_value(data->hc_x, col, row);
    hc_y = N_get_array_2d_d_value(data->hc_y, col, row);
    hc_xw = N_get_array_2d_d_value(data->hc_x, col - 1, row);
    hc_xe = N_get_array_2d_d_value(data->hc_x, col + 1, row);
    hc_yn = N_get_array_2d_d_value(data->hc_y, col, row - 1);
    hc_ys = N_get_array_2d_d_value(data->hc_y, col, row + 1);

    /* calculate the transmissivities */
    T_w = N_calc_harmonic_mean(hc_xw, hc_x) * z_w;
    T_e = N_calc_harmonic_mean(hc_xe, hc_x) * z_e;
    T_n = N_calc_harmonic_mean(hc_yn, hc_y) * z_n;
    T_s = N_calc_harmonic_mean(hc_ys, hc_y) * z_s;

    /* Compute the river leakage, this is an explicit method
     * Influent and effluent flow is computed.
     */
    if (data->river_leak &&
	(N_get_array_2d_d_value(data->river_leak, col, row) != 0) &&
            N_get_array_2d_d_value(data->river_bed, col, row) <= top) {
        /* Groundwater surface is above the river bed*/
	if (hc > N_get_array_2d_d_value(data->river_bed, col, row)) {
	    river_vect = N_get_array_2d_d_value(data->river_head, col, row) *
		N_get_array_2d_d_value(data->river_leak, col, row);
	    river_mat = N_get_array_2d_d_value(data->river_leak, col, row);
	} /* Groundwater surface is below the river bed */
	else if (hc < N_get_array_2d_d_value(data->river_bed, col, row)) {
	    river_vect = (N_get_array_2d_d_value(data->river_head, col, row) -
			  N_get_array_2d_d_value(data->river_bed, col, row))
		* N_get_array_2d_d_value(data->river_leak, col, row);
	    river_mat = 0;
	}
    }

    /* compute the drainage, this is an explicit method
     * Drainage is only enabled, if the drain bed is lower the groundwater surface
     */
    if (data->drain_leak &&
	(N_get_array_2d_d_value(data->drain_leak, col, row) != 0) &&
            N_get_array_2d_d_value(data->drain_bed, col, row) <= top) {
	if (hc > N_get_array_2d_d_value(data->drain_bed, col, row)) {
	    drain_vect = N_get_array_2d_d_value(data->drain_bed, col, row) *
		N_get_array_2d_d_value(data->drain_leak, col, row);
	    drain_mat = N_get_array_2d_d_value(data->drain_leak, col, row);
	}
	else if (hc <= N_get_array_2d_d_value(data->drain_bed, col, row)) {
	    drain_vect = 0;
	    drain_mat = 0;
	}
    }

    /*mass balance center cell to western cell */
    W = -1 * T_w * dy / dx;
    /*mass balance center cell to eastern cell */
    E = -1 * T_e * dy / dx;
    /*mass balance center cell to northern cell */
    N = -1 * T_n * dx / dy;
    /*mass balance center cell to southern cell */
    S = -1 * T_s * dx / dy;

    /*the diagonal entry of the matrix */
    C = -1 * (W + E + N + S -  Az *Ss / data->dt - river_mat * Az -
	      drain_mat * Az);

    /*the entry in the right side b of Ax = b */
    V = (q + hc_start * Az * Ss / data->dt) + r + river_vect * Az +
	drain_vect * Az;

    G_debug(5, "N_callback_gwflow_2d: called [%i][%i]", row, col);

    /*create the 5 point star entries */
    mat_pos = N_create_5star(C, W, E, N, S, V);

    return mat_pos;
}



/* *************************************************************** */
/* ****************** N_gwflow_2d_calc_water_budget ************** */
/* *************************************************************** */
/*!
 * \brief This function computes the water budget of the entire groundwater
 *
 * The water budget is calculated for each active and dirichlet cell from
 * its surrounding neighbours. This is based on the 5 star mass balance computation
 * of N_callback_gwflow_2d and the gradient of the water heights in the cells.
 * The sum of the water budget of each active/dirichlet cell must be near zero
 * due the effect of numerical inaccuracy of cpu's.
 *
 * \param gwdata N_gwflow_data2d *
 * \param geom N_geom_data *
 * \param budget N_array_2d
 * \return void
 *
 * */
void
N_gwflow_2d_calc_water_budget(N_gwflow_data2d * data, N_geom_data * geom, N_array_2d * budget)
{
    int y, x, stat;
    double h, hc;
    double val;
    double sum;
    N_data_star *dstar;

    int rows = data->status->rows;
    int cols = data->status->cols;

    sum = 0;

    for (y = 0; y < rows; y++) {
	G_percent(y, rows - 1, 10);
	for (x = 0; x < cols; x++) {
	    stat = N_get_array_2d_c_value(data->status, x, y);

            val = 0.0;

	    if (stat != N_CELL_INACTIVE ) {	/*all active/dirichlet cells */

                /* Compute the flow parameter */
                dstar = N_callback_gwflow_2d(data, geom, x, y);
                /* Compute the gradient in each direction pointing from the center */
                hc = N_get_array_2d_d_value(data->phead, x, y);

                if((int)N_get_array_2d_d_value(data->status, x + 1, y    ) != N_CELL_INACTIVE) {
                    h = N_get_array_2d_d_value(data->phead,  x + 1, y);
                    val += dstar->E * (hc - h);
                }
                if((int)N_get_array_2d_d_value(data->status, x - 1, y    ) != N_CELL_INACTIVE) {
                    h = N_get_array_2d_d_value(data->phead,  x - 1, y);
                    val += dstar->W * (hc - h);
                }
                if((int)N_get_array_2d_d_value(data->status, x    , y + 1) != N_CELL_INACTIVE) {
                    h = N_get_array_2d_d_value(data->phead,  x    , y + 1);
                    val += dstar->S * (hc - h);
                }
                if((int)N_get_array_2d_d_value(data->status, x    , y - 1) != N_CELL_INACTIVE) {
                    h = N_get_array_2d_d_value(data->phead,  x    , y - 1);
                    val += dstar->N * (hc - h);
                }

                sum += val;

                G_free(dstar);
	    }
	    else {
		Rast_set_null_value(&val, 1, DCELL_TYPE);
	    }
	    N_put_array_2d_d_value(budget, x, y, val);
	}
    }

    if(fabs(sum) < 0.0000000001)
        G_message(_("The total sum of the water budget: %g\n"), sum);
    else
        G_warning(_("The total sum of the water budget is significantly larger then 0: %g\n"), sum);

    return;
}
