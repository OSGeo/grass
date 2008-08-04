/*
   Chris Rewerts, Agricultural Engineering, Purdue University
   April 1991
 */

#include "edit.h"
int draw_grid(void)
{
    double U_start;
    double D_south, D_west;
    double D_north, D_east;
    double U_to_D_xconv, U_to_D_yconv;
    double U_west, U_south;
    double U_east, U_north;
    double U_x, U_y;
    int D_x, D_y;
    double ew_res, ns_res;


    /* draw grid */

    ew_res = real_window.ew_res;
    ns_res = real_window.ns_res;
    U_west = D_get_u_west();
    U_east = D_get_u_east();
    U_south = D_get_u_south();
    U_north = D_get_u_north();
    U_to_D_xconv = D_get_u_to_d_xconv();
    U_to_D_yconv = D_get_u_to_d_yconv();

    D_south = D_get_d_south();
    D_north = D_get_d_north();
    D_east = D_get_d_east();
    D_west = D_get_d_west();

    R_standard_color(grid_color);

    /* Draw vertical grids */
    U_start = U_east;
    for (U_x = U_start; U_x >= U_west; U_x -= ew_res) {
	D_x = (int)((U_x - U_west) * U_to_D_xconv + D_west);
	R_move_abs(D_x, (int)D_south);
	R_cont_abs(D_x, (int)D_north);
    }

    /* Draw horizontal grids */
    U_start = U_north;
    for (U_y = U_start; U_y >= U_south; U_y -= ns_res) {
	D_y = (int)((U_south - U_y) * U_to_D_yconv + D_south);
	R_move_abs((int)D_west, D_y);
	R_cont_abs((int)D_east, D_y);
    }

    R_stabilize();

    return 0;
}
