#include <math.h>
#include <grass/raster3d.h>

#include "voxel_traversal.h"

void traverse(RASTER3D_Region * region, double *start, double *end,
	      int **coordinates, int *size, int *coor_count)
{
    double dx, dy, dz;
    int step_x, step_y, step_z;
    int x, y, z;
    int x_end, y_end, z_end;
    double t_delta_x, t_delta_y, t_delta_z;
    double t_max_x, t_max_y, t_max_z;
    double xtmp, ytmp, ztmp;
    int count;

    /* initialize */
    dx = end[0] - start[0];
    dy = end[1] - start[1];
    dz = end[2] - start[2];

    step_x = start[0] < end[0] ? 1 : -1;
    step_y = start[1] < end[1] ? 1 : -1;
    step_z = start[2] < end[2] ? 1 : -1;

    x = (int)floor((start[0] - region->west) / region->ew_res);
    y = (int)floor((start[1] - region->south) / region->ns_res);
    z = (int)floor((start[2] - region->bottom) / region->tb_res);
    x_end = (int)floor((end[0] - region->west) / region->ew_res);
    y_end = (int)floor((end[1] - region->south) / region->ns_res);
    z_end = (int)floor((end[2] - region->bottom) / region->tb_res);

    t_delta_x = fabs(region->ew_res / (dx != 0 ? dx : 1e6));
    t_delta_y = fabs(region->ns_res / (dy != 0 ? dy : 1e6));
    t_delta_z = fabs(region->tb_res / (dz != 0 ? dz : 1e6));

    xtmp = (start[0] - region->west) / region->ew_res;
    ytmp = (start[1] - region->south) / region->ns_res;
    ztmp = (start[2] - region->bottom) / region->tb_res;

    if (step_x > 0)
	t_max_x = t_delta_x * (1.0 - (xtmp - floor(xtmp)));
    else
	t_max_x = t_delta_x * (xtmp - floor(xtmp));
    if (step_y > 0)
	t_max_y = t_delta_y * (1.0 - (ytmp - floor(ytmp)));
    else
	t_max_y = t_delta_y * (ytmp - floor(ytmp));
    if (step_z > 0)
	t_max_z = t_delta_z * (1.0 - (ztmp - floor(ztmp)));
    else
	t_max_z = t_delta_z * (ztmp - floor(ztmp));

    count = 0;
    while (TRUE) {
	if (t_max_x < t_max_y) {
	    if (t_max_x < t_max_z) {
		t_max_x = t_max_x + t_delta_x;
		x = x + step_x;
	    }
	    else {
		t_max_z = t_max_z + t_delta_z;
		z = z + step_z;
	    }
	}
	else {
	    if (t_max_y < t_max_z) {
		t_max_y = t_max_y + t_delta_y;
		y = y + step_y;
	    }
	    else {
		t_max_z = t_max_z + t_delta_z;
		z = z + step_z;
	    }
	}
	if ((x == x_end && y == y_end && z == z_end) ||
	    /* just to make sure it breaks */
	    (step_x * (x - x_end) > 0 || step_y * (y - y_end) > 0 ||
	     step_z * (z - z_end) > 0))

	    break;

	(*coordinates)[count * 3 + 0] = x;
	(*coordinates)[count * 3 + 1] = region->rows - y - 1;
	(*coordinates)[count * 3 + 2] = z;
	count++;

	/* reallocation for cases when the steps would be too big */
	if (*size <= count) {
	    *size = 2 * (*size);
	    *coordinates = G_realloc(*coordinates, (*size) * 3 * sizeof(int));
	}
    }
    *coor_count = count;
}
