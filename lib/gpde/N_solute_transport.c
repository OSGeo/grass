
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      solute transport in porous media
* 		part of the gpde library
*
* COPYRIGHT:    (C) 2007 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <math.h>
#include <grass/N_solute_transport.h>

/* ************************************************************************* *
 * ************************************************************************* *
 * ************************************************************************* */
/*! \brief This is just a placeholder
 *
 * */
N_data_star *N_callback_solute_transport_3d(void *solutedata,
					    N_geom_data * geom, int col,
					    int row, int depth)
{
    double Df_e = 0, Df_w = 0, Df_n = 0, Df_s = 0, Df_t = 0, Df_b = 0;
    double dx, dy, dz, Az;
    double diff_x, diff_y, diff_z;
    double diff_xw, diff_yn;
    double diff_xe, diff_ys;
    double diff_zt, diff_zb;
    double cin = 0, cg, cg_start;
    double R, nf, cs, q;
    double C, W, E, N, S, T, B, V;
    double vw = 0, ve = 0, vn = 0, vs = 0, vt = 0, vb = 0;
    double Ds_w = 0, Ds_e = 0, Ds_n = 0, Ds_s = 0, Ds_t = 0, Ds_b = 0;
    double Dw = 0, De = 0, Dn = 0, Ds = 0, Dt = 0, Db = 0;
    double rw = 0.5, re = 0.5, rn = 0.5, rs = 0.5, rt = 0.5, rb = 0.5;

    N_solute_transport_data3d *data = NULL;
    N_data_star *mat_pos;
    N_gradient_3d grad;

    /*cast the void pointer to the right data structure */
    data = (N_solute_transport_data3d *) solutedata;

    N_get_gradient_3d(data->grad, &grad, col, row, depth);

    dx = geom->dx;
    dy = geom->dy;
    dz = geom->dz;
    Az = N_get_geom_data_area_of_cell(geom, row);

    /*read the data from the arrays */
    cg_start = N_get_array_3d_d_value(data->c_start, col, row, depth);
    cg = N_get_array_3d_d_value(data->c, col, row, depth);

    /*get the surrounding diffusion tensor entries */
    diff_x = N_get_array_3d_d_value(data->diff_x, col, row, depth);
    diff_y = N_get_array_3d_d_value(data->diff_y, col, row, depth);
    diff_z = N_get_array_3d_d_value(data->diff_z, col, row, depth);
    diff_xw = N_get_array_3d_d_value(data->diff_x, col - 1, row, depth);
    diff_xe = N_get_array_3d_d_value(data->diff_x, col + 1, row, depth);
    diff_yn = N_get_array_3d_d_value(data->diff_y, col, row - 1, depth);
    diff_ys = N_get_array_3d_d_value(data->diff_y, col, row + 1, depth);
    diff_zt = N_get_array_3d_d_value(data->diff_z, col, row, depth + 1);
    diff_zb = N_get_array_3d_d_value(data->diff_z, col, row, depth - 1);

    /* calculate the diffusion on the cell borders using the harmonical mean */
    Df_w = N_calc_harmonic_mean(diff_xw, diff_x);
    Df_e = N_calc_harmonic_mean(diff_xe, diff_x);
    Df_n = N_calc_harmonic_mean(diff_yn, diff_y);
    Df_s = N_calc_harmonic_mean(diff_ys, diff_y);
    Df_t = N_calc_harmonic_mean(diff_zt, diff_z);
    Df_b = N_calc_harmonic_mean(diff_zb, diff_z);

    /* calculate the dispersion */
    /*todo */

    /* calculate the velocity parts  with full upwinding scheme */
    vw = grad.WC;
    ve = grad.EC;
    vn = grad.NC;
    vs = grad.SC;
    vt = grad.TC;
    vb = grad.BC;

    /* put the diffusion and dispersion together */
    Dw = ((Df_w + Ds_w)) / dx;
    De = ((Df_e + Ds_e)) / dx;
    Dn = ((Df_n + Ds_n)) / dy;
    Ds = ((Df_s + Ds_s)) / dy;
    Dt = ((Df_t + Ds_t)) / dz;
    Db = ((Df_b + Ds_b)) / dz;

    rw = N_exp_upwinding(-1 * vw, dx, Dw);
    re = N_exp_upwinding(ve, dx, De);
    rs = N_exp_upwinding(-1 * vs, dy, Ds);
    rn = N_exp_upwinding(vn, dy, Dn);
    rb = N_exp_upwinding(-1 * vb, dz, Dn);
    rt = N_exp_upwinding(vt, dz, Dn);

    /*mass balance center cell to western cell */
    W = -1 * (Dw) * dy * dz - vw * (1 - rw) * dy * dz;
    /*mass balance center cell to eastern cell */
    E = -1 * (De) * dy * dz + ve * (1 - re) * dy * dz;
    /*mass balance center cell to southern cell */
    S = -1 * (Ds) * dx * dz - vs * (1 - rs) * dx * dz;
    /*mass balance center cell to northern cell */
    N = -1 * (Dn) * dx * dz + vn * (1 - rn) * dx * dz;
    /*mass balance center cell to bottom cell */
    B = -1 * (Db) * Az - vb * (1 - rb) * Az;
    /*mass balance center cell to top cell */
    T = -1 * (Dt) * Az + vt * (1 - rt) * Az;

    /* Retardation */
    R = N_get_array_3d_d_value(data->R, col, row, depth);
    /* Inner sources */
    cs = N_get_array_3d_d_value(data->cs, col, row, depth);
    /* effective porosity */
    nf = N_get_array_3d_d_value(data->nf, col, row, depth);
    /* groundwater sources and sinks */
    q = N_get_array_3d_d_value(data->q, col, row, depth);
    /* concentration of influent water */
    cin = N_get_array_3d_d_value(data->cin, col, row, depth);

    /*the diagonal entry of the matrix */
    C = ((Dw - vw) * dy * dz +
	 (De + ve) * dy * dz +
	 (Ds - vs) * dx * dz +
	 (Dn + vn) * dx * dz +
	 (Db - vb) * Az + (Dt + vt) * Az + Az * dz * R / data->dt - q / nf);

    /*the entry in the right side b of Ax = b */
    V = (cs + cg_start * Az * dz * R / data->dt - q / nf * cin);

    /*
     * printf("nf %g\n", nf);
     * printf("q %g\n", q);
     * printf("cs %g\n", cs);
     * printf("cin %g\n", cin);
     * printf("cg %g\n", cg);
     * printf("cg_start %g\n", cg_start);
     * printf("Az %g\n", Az);
     * printf("z %g\n", z);
     * printf("R %g\n", R);
     * printf("dt %g\n", data->dt);
     */
    G_debug(6, "N_callback_solute_transport_3d: called [%i][%i][%i]", row,
	    col, depth);

    /*create the 7 point star entries */
    mat_pos = N_create_7star(C, W, E, N, S, T, B, V);

    return mat_pos;
}

/* ************************************************************************* *
 * ************************************************************************* *
 * ************************************************************************* */
/*!
 * \brief This callback function creates the mass balance of a 5 point star
 *
 * The mass balance is based on the common solute transport equation:
 *
 * \f[\frac{\partial c_g}{\partial t} R = \nabla \cdot ({\bf D} \nabla c_g - {\bf u} c_g) + \sigma + \frac{q}{n_f}(c_g - c_in) \f]
 *
 * This equation is discretizised with the finite volume method in two dimensions.
 *
 *
 * \param solutedata  * N_solute_transport_data2d - a void pointer to the data structure
 * \param geom N_geom_data *
 * \param col   int
 * \param row   int
 * \return N_data_star * - a five point data star
 *
 * */
N_data_star *N_callback_solute_transport_2d(void *solutedata,
					    N_geom_data * geom, int col,
					    int row)
{
    double Df_e = 0, Df_w = 0, Df_n = 0, Df_s = 0;
    double z_e = 0, z_w = 0, z_n = 0, z_s = 0;
    double dx, dy, Az;
    double diff_x, diff_y;
    double disp_x, disp_y;
    double z;
    double diff_xw, diff_yn;
    double disp_xw, disp_yn;
    double z_xw, z_yn;
    double diff_xe, diff_ys;
    double disp_xe, disp_ys;
    double z_xe, z_ys;
    double cin = 0, cg, cg_start;
    double R, nf, cs, q;
    double C, W, E, N, S, V, NE, NW, SW, SE;
    double vw = 0, ve = 0, vn = 0, vs = 0;
    double Ds_w = 0, Ds_e = 0, Ds_n = 0, Ds_s = 0;
    double Dw = 0, De = 0, Dn = 0, Ds = 0;
    double rw = 0.5, re = 0.5, rn = 0.5, rs = 0.5;

    N_solute_transport_data2d *data = NULL;
    N_data_star *mat_pos;
    N_gradient_2d grad;

    /*cast the void pointer to the right data structure */
    data = (N_solute_transport_data2d *) solutedata;

    N_get_gradient_2d(data->grad, &grad, col, row);

    dx = geom->dx;
    dy = geom->dy;
    Az = N_get_geom_data_area_of_cell(geom, row);

    /*read the data from the arrays */
    cg_start = N_get_array_2d_d_value(data->c_start, col, row);
    cg = N_get_array_2d_d_value(data->c, col, row);

    /* calculate the cell height */
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

    /*geometrical mean of cell height */
    z_w = N_calc_geom_mean(z_xw, z);
    z_e = N_calc_geom_mean(z_xe, z);
    z_n = N_calc_geom_mean(z_yn, z);
    z_s = N_calc_geom_mean(z_ys, z);

    /*get the surrounding diffusion tensor entries */
    diff_x = N_get_array_2d_d_value(data->diff_x, col, row);
    diff_y = N_get_array_2d_d_value(data->diff_y, col, row);
    diff_xw = N_get_array_2d_d_value(data->diff_x, col - 1, row);
    diff_xe = N_get_array_2d_d_value(data->diff_x, col + 1, row);
    diff_yn = N_get_array_2d_d_value(data->diff_y, col, row - 1);
    diff_ys = N_get_array_2d_d_value(data->diff_y, col, row + 1);

    /* calculate the diffusion at the cell borders using the harmonical mean */
    Df_w = N_calc_harmonic_mean(diff_xw, diff_x);
    Df_e = N_calc_harmonic_mean(diff_xe, diff_x);
    Df_n = N_calc_harmonic_mean(diff_yn, diff_y);
    Df_s = N_calc_harmonic_mean(diff_ys, diff_y);

    /* calculate the dispersion */
    /*get the surrounding dispersion tensor entries */
    disp_x = N_get_array_2d_d_value(data->disp_xx, col, row);
    disp_y = N_get_array_2d_d_value(data->disp_yy, col, row);
    if (N_get_array_2d_d_value(data->status, col - 1, row) ==
	N_CELL_TRANSMISSION) {
	disp_xw = disp_x;
    }
    else {
	disp_xw = N_get_array_2d_d_value(data->disp_xx, col - 1, row);
    }
    if (N_get_array_2d_d_value(data->status, col + 1, row) ==
	N_CELL_TRANSMISSION) {
	disp_xe = disp_x;
    }
    else {
	disp_xe = N_get_array_2d_d_value(data->disp_xx, col + 1, row);
    }
    if (N_get_array_2d_d_value(data->status, col, row - 1) ==
	N_CELL_TRANSMISSION) {
	disp_yn = disp_y;
    }
    else {
	disp_yn = N_get_array_2d_d_value(data->disp_yy, col, row - 1);
    }
    if (N_get_array_2d_d_value(data->status, col, row + 1) ==
	N_CELL_TRANSMISSION) {
	disp_ys = disp_y;
    }
    else {
	disp_ys = N_get_array_2d_d_value(data->disp_yy, col, row + 1);
    }

    /* calculate the dispersion at the cell borders using the harmonical mean */
    Ds_w = N_calc_harmonic_mean(disp_xw, disp_x);
    Ds_e = N_calc_harmonic_mean(disp_xe, disp_x);
    Ds_n = N_calc_harmonic_mean(disp_yn, disp_y);
    Ds_s = N_calc_harmonic_mean(disp_ys, disp_y);

    /* put the diffusion and dispersion together */
    Dw = ((Df_w + Ds_w)) / dx;
    De = ((Df_e + Ds_e)) / dx;
    Ds = ((Df_s + Ds_s)) / dy;
    Dn = ((Df_n + Ds_n)) / dy;

    vw = -1.0 * grad.WC;
    ve = grad.EC;
    vs = -1.0 * grad.SC;
    vn = grad.NC;

    if (data->stab == N_UPWIND_FULL) {
	rw = N_full_upwinding(vw, dx, Dw);
	re = N_full_upwinding(ve, dx, De);
	rs = N_full_upwinding(vs, dy, Ds);
	rn = N_full_upwinding(vn, dy, Dn);
    }
    else if (data->stab == N_UPWIND_EXP) {
	rw = N_exp_upwinding(vw, dx, Dw);
	re = N_exp_upwinding(ve, dx, De);
	rs = N_exp_upwinding(vs, dy, Ds);
	rn = N_exp_upwinding(vn, dy, Dn);
    }

    /*mass balance center cell to western cell */
    W = -1 * (Dw) * dy * z_w + vw * (1 - rw) * dy * z_w;
    /*mass balance center cell to eastern cell */
    E = -1 * (De) * dy * z_e + ve * (1 - re) * dy * z_e;
    /*mass balance center cell to southern cell */
    S = -1 * (Ds) * dx * z_s + vs * (1 - rs) * dx * z_s;
    /*mass balance center cell to northern cell */
    N = -1 * (Dn) * dx * z_n + vn * (1 - rn) * dx * z_n;

    NW = 0.0;
    SW = 0.0;
    NE = 0.0;
    SE = 0.0;

    /* Retardation */
    R = N_get_array_2d_d_value(data->R, col, row);
    /* Inner sources */
    cs = N_get_array_2d_d_value(data->cs, col, row);
    /* effective porosity */
    nf = N_get_array_2d_d_value(data->nf, col, row);
    /* groundwater sources and sinks */
    q = N_get_array_2d_d_value(data->q, col, row);
    /* concentration of influent water */
    cin = N_get_array_2d_d_value(data->cin, col, row);

    /*the diagonal entry of the matrix */
    C = (Dw + vw * rw) * dy * z_w +
	(De + ve * re) * dy * z_e +
	(Ds + vs * rs) * dx * z_s +
	(Dn + vn * rn) * dx * z_n + Az * z * R / data->dt - q / nf;

    /*the entry in the right side b of Ax = b */
    V = (cs + cg_start * Az * z * R / data->dt + q / nf * cin);

    /*
       fprintf(stderr, "nf %g\n", nf);
       fprintf(stderr, "q %g\n", q);
       fprintf(stderr, "cs %g\n", cs);
       fprintf(stderr, "cin %g\n", cin);
       fprintf(stderr, "cg %g\n", cg);
       fprintf(stderr, "cg_start %g\n", cg_start);
       fprintf(stderr, "Az %g\n", Az);
       fprintf(stderr, "z %g\n", z);
       fprintf(stderr, "R %g\n", R);
       fprintf(stderr, "dt %g\n", data->dt);
     */

    G_debug(6, "N_callback_solute_transport_2d: called [%i][%i]", row, col);

    /*create the 9 point star entries */
    mat_pos = N_create_9star(C, W, E, N, S, NW, SW, NE, SE, V);

    return mat_pos;
}

/* ************************************************************************* *
 * ************************************************************************* *
 * ************************************************************************* */
/*!
 * \brief Alllocate memory for the solute transport data structure in three dimensions
 *
 * The solute transport data structure will be allocated including
 * all appendant 3d arrays. The offset for the 3d arrays is one
 * to establish homogeneous Neumann boundary conditions at the calculation area border.
 * This data structure is used to create a linear equation system based on the computation of
 * solute transport in porous media with the finite volume method.
 *
 * \param cols   int
 * \param rows   int
 * \param depths int
 * \return N_solute_transport_data3d *
 * */

N_solute_transport_data3d *N_alloc_solute_transport_data3d(int cols, int rows,
							   int depths)
{
    N_solute_transport_data3d *data = NULL;

    data =
	(N_solute_transport_data3d *) G_calloc(1,
					       sizeof
					       (N_solute_transport_data3d));

    data->c = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->c_start = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->status = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->diff_x = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->diff_y = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->diff_z = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->q = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->cs = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->R = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->nf = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->cin = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);

    /*Allocate the dispersivity tensor */
    data->disp_xx = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->disp_yy = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->disp_zz = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->disp_xy = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->disp_xz = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);
    data->disp_yz = N_alloc_array_3d(cols, rows, depths, 1, DCELL_TYPE);


    data->grad = N_alloc_gradient_field_3d(cols, rows, depths);
    data->stab = N_UPWIND_EXP;

    return data;
}

/* ************************************************************************* *
 * ************************************************************************* *
 * ************************************************************************* */
/*!
 * \brief Alllocate memory for the solute transport data structure in two dimensions
 *
 * The solute transport data structure will be allocated including
 * all appendant 2d arrays. The offset for the 2d arrays is one
 * to establish homogeneous Neumann boundary conditions at the calculation area border.
 * This data structure is used to create a linear equation system based on the computation of
 * solute transport in porous media with the finite volume method.
 *
 * \param cols   int
 * \param rows   int
 * \return N_solute_transport_data2d *
 * */


N_solute_transport_data2d *N_alloc_solute_transport_data2d(int cols, int rows)
{
    N_solute_transport_data2d *data = NULL;

    data =
	(N_solute_transport_data2d *) G_calloc(1,
					       sizeof
					       (N_solute_transport_data2d));

    data->c = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->c_start = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->status = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->diff_x = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->diff_y = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->q = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->cs = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->R = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->nf = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->cin = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->top = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->bottom = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);

    /*Allocate the dispersivity tensor */
    data->disp_xx = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->disp_yy = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);
    data->disp_xy = N_alloc_array_2d(cols, rows, 1, DCELL_TYPE);

    data->grad = N_alloc_gradient_field_2d(cols, rows);
    data->stab = N_UPWIND_EXP;

    return data;
}

/* ************************************************************************* *
 * ************************************************************************* *
 * ************************************************************************* */
/*!
 * \brief Release the memory of the solute transport data structure in three dimensions
 *
 * \param data N_solute_transport_data2d *
 * \return void *
 * */
void N_free_solute_transport_data3d(N_solute_transport_data3d * data)
{
    N_free_array_3d(data->c);
    N_free_array_3d(data->c_start);
    N_free_array_3d(data->status);
    N_free_array_3d(data->diff_x);
    N_free_array_3d(data->diff_y);
    N_free_array_3d(data->diff_z);
    N_free_array_3d(data->q);
    N_free_array_3d(data->cs);
    N_free_array_3d(data->R);
    N_free_array_3d(data->nf);
    N_free_array_3d(data->cin);

    N_free_array_3d(data->disp_xx);
    N_free_array_3d(data->disp_yy);
    N_free_array_3d(data->disp_zz);
    N_free_array_3d(data->disp_xy);
    N_free_array_3d(data->disp_xz);
    N_free_array_3d(data->disp_yz);

    G_free(data);

    data = NULL;

    return;
}

/* ************************************************************************* *
 * ************************************************************************* *
 * ************************************************************************* */
/*!
 * \brief Release the memory of the solute transport data structure in two dimensions
 *
 * \param data N_solute_transport_data2d *
 * \return void *
 * */
void N_free_solute_transport_data2d(N_solute_transport_data2d * data)
{
    N_free_array_2d(data->c);
    N_free_array_2d(data->c_start);
    N_free_array_2d(data->status);
    N_free_array_2d(data->diff_x);
    N_free_array_2d(data->diff_y);
    N_free_array_2d(data->q);
    N_free_array_2d(data->cs);
    N_free_array_2d(data->R);
    N_free_array_2d(data->nf);
    N_free_array_2d(data->cin);
    N_free_array_2d(data->top);
    N_free_array_2d(data->bottom);

    N_free_array_2d(data->disp_xx);
    N_free_array_2d(data->disp_yy);
    N_free_array_2d(data->disp_xy);

    G_free(data);

    data = NULL;

    return;
}

/*!
 * \brief Compute the transmission boundary condition in 2d
 *
 * This function calculates the transmission boundary condition
 * for each cell with status N_CELL_TRANSMISSION. The surrounding
 * gradient field is used to verfiy the flow direction. If a flow
 * goes into a cell, the concentration (data->c) from the neighbour cell is
 * added to the transmission cell. If the flow from several neighbour 
 * cells goes into the cell, the concentration mean is calculated.
 * 
 * The new concentrations are written into the data->c_start array,
 * so they can be handled by the matrix assembling function.
 *
 * \param data N_solute_transport_data2d *
 * \return void *
 * */
void N_calc_solute_transport_transmission_2d(N_solute_transport_data2d * data)
{
    int i, j, count = 1;
    int cols, rows;
    double c;
    N_gradient_2d grad;

    cols = data->grad->cols;
    rows = data->grad->rows;

    G_debug(2,
	    "N_calc_solute_transport_transmission_2d: calculating transmission boundary");

    for (j = 0; j < rows; j++) {
	for (i = 0; i < cols; i++) {
	    if (N_get_array_2d_d_value(data->status, i, j) ==
		N_CELL_TRANSMISSION) {
		count = 0;
		/*get the gradient neighbours */
		N_get_gradient_2d(data->grad, &grad, i, j);
		c = 0;
		/*
		   c = N_get_array_2d_d_value(data->c_start, i, j);
		   if(c > 0)
		   count++;
		 */

		if (grad.WC > 0 &&
		    !N_is_array_2d_value_null(data->c, i - 1, j)) {
		    c += N_get_array_2d_d_value(data->c, i - 1, j);
		    count++;
		}
		if (grad.EC < 0 &&
		    !N_is_array_2d_value_null(data->c, i + 1, j)) {
		    c += N_get_array_2d_d_value(data->c, i + 1, j);
		    count++;
		}
		if (grad.NC < 0 &&
		    !N_is_array_2d_value_null(data->c, i, j - 1)) {
		    c += N_get_array_2d_d_value(data->c, i, j - 1);
		    count++;
		}
		if (grad.SC > 0 &&
		    !N_is_array_2d_value_null(data->c, i, j + 1)) {
		    c += N_get_array_2d_d_value(data->c, i, j + 1);
		    count++;
		}
		if (count != 0)
		    c = c / (double)count;
		/*make sure it is not NAN */
		if (c > 0 || c == 0 || c < 0)
		    N_put_array_2d_d_value(data->c_start, i, j, c);
	    }
	}
    }

    return;
}

/*!
 * \brief Compute the dispersivity tensor based on the solute transport data in 2d
 *
 * The dispersivity tensor is stored in the data structure.
 * To compute the dispersivity tensor, the dispersivity lentghs and the gradient field
 * must be present.
 *
 * This is just a simple tensor computation which should be extended.
 *
 * \todo Change the tensor calculation to a mor realistic algorithm 
 *
 * \param data N_solute_transport_data2d *
 * \return void *
 * */
void N_calc_solute_transport_disptensor_2d(N_solute_transport_data2d * data)
{
    int i, j;
    int cols, rows;
    double vx, vy, vv;
    double disp_xx, disp_yy, disp_xy;
    N_gradient_2d grad;

    cols = data->grad->cols;
    rows = data->grad->rows;

    G_debug(2,
	    "N_calc_solute_transport_disptensor_2d: calculating the dispersivity tensor");

    for (j = 0; j < rows; j++) {
	for (i = 0; i < cols; i++) {

	    disp_xx = 0;
	    disp_yy = 0;
	    disp_xy = 0;

	    /*get the gradient neighbours */
	    N_get_gradient_2d(data->grad, &grad, i, j);
	    vx = (grad.WC + grad.EC) / 2;
	    vy = (grad.NC + grad.SC) / 2;
	    vv = sqrt(vx * vx + vy * vy);

	    if (vv != 0) {
		disp_xx = data->al * vx * vx / vv + data->at * vy * vy / vv;
		disp_yy = data->at * vx * vx / vv + data->al * vy * vy / vv;
		disp_xy = (data->al - data->at) * vx * vy / vv;
	    }

	    G_debug(5,
		    "N_calc_solute_transport_disptensor_2d: [%i][%i] disp_xx %g disp_yy %g disp_xy %g",
		    i, j, disp_xx, disp_yy, disp_xy);
	    N_put_array_2d_d_value(data->disp_xx, i, j, disp_xx);
	    N_put_array_2d_d_value(data->disp_yy, i, j, disp_yy);
	    N_put_array_2d_d_value(data->disp_xy, i, j, disp_xy);
	}
    }

    return;
}

/*!
 * \brief Compute the dispersivity tensor based on the solute transport data in 3d
 *
 * The dispersivity tensor is stored in the data structure.
 * To compute the dispersivity tensor, the dispersivity lentghs and the gradient field
 * must be present.
 * 
 * This is just a simple tensor computation which should be extended.
 *
 * \todo Change the tensor calculation to a mor realistic algorithm 
 *
 * \param data N_solute_transport_data3d *
 * \return void *
 * */
void N_calc_solute_transport_disptensor_3d(N_solute_transport_data3d * data)
{
    int i, j, k;
    int cols, rows, depths;
    double vx, vy, vz, vv;
    double disp_xx, disp_yy, disp_zz, disp_xy, disp_xz, disp_yz;
    N_gradient_3d grad;

    cols = data->grad->cols;
    rows = data->grad->rows;
    depths = data->grad->depths;

    G_debug(2,
	    "N_calc_solute_transport_disptensor_3d: calculating the dispersivity tensor");

    for (k = 0; k < depths; k++) {
	for (j = 0; j < rows; j++) {
	    for (i = 0; i < cols; i++) {
		disp_xx = 0;
		disp_yy = 0;
		disp_zz = 0;
		disp_xy = 0;
		disp_xz = 0;
		disp_yz = 0;

		/*get the gradient neighbours */
		N_get_gradient_3d(data->grad, &grad, i, j, k);
		vx = (grad.WC + grad.EC) / 2;
		vy = (grad.NC + grad.SC) / 2;
		vz = (grad.BC + grad.TC) / 2;
		vv = sqrt(vx * vx + vy * vy + vz * vz);

		if (vv != 0) {
		    disp_xx =
			data->al * vx * vx / vv + data->at * vy * vy / vv +
			data->at * vz * vz / vv;
		    disp_yy =
			data->at * vx * vx / vv + data->al * vy * vy / vv +
			data->at * vz * vz / vv;
		    disp_zz =
			data->at * vx * vx / vv + data->at * vy * vy / vv +
			data->al * vz * vz / vv;
		    disp_xy = (data->al - data->at) * vx * vy / vv;
		    disp_xz = (data->al - data->at) * vx * vz / vv;
		    disp_yz = (data->al - data->at) * vy * vz / vv;
		}

		G_debug(5,
			"N_calc_solute_transport_disptensor_3d: [%i][%i][%i] disp_xx %g disp_yy %g disp_zz %g  disp_xy %g disp_xz %g disp_yz %g ",
			i, j, k, disp_xx, disp_yy, disp_zz, disp_xy, disp_xz,
			disp_yz);
		N_put_array_3d_d_value(data->disp_xx, i, j, k, disp_xx);
		N_put_array_3d_d_value(data->disp_yy, i, j, k, disp_yy);
		N_put_array_3d_d_value(data->disp_zz, i, j, k, disp_zz);
		N_put_array_3d_d_value(data->disp_xy, i, j, k, disp_xy);
		N_put_array_3d_d_value(data->disp_xz, i, j, k, disp_xz);
		N_put_array_3d_d_value(data->disp_yz, i, j, k, disp_yz);
	    }
	}
    }

    return;
}
