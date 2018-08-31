#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <grass/raster.h>
#include <grass/raster3d.h>

/*---------------------------------------------------------------------------*/

static int verifyVolumeVertices(void *map, double v[2][2][2][3])
{
    if (!(Rast3d_is_valid_location(map, v[0][0][0][0], v[0][0][0][1],
			      v[0][0][0][2]) &&
	  Rast3d_is_valid_location(map, v[0][0][1][0], v[0][0][1][1],
			      v[0][0][1][2]) &&
	  Rast3d_is_valid_location(map, v[0][1][0][0], v[0][1][0][1],
			      v[0][1][0][2]) &&
	  Rast3d_is_valid_location(map, v[0][1][1][0], v[0][1][1][1],
			      v[0][1][1][2]) &&
	  Rast3d_is_valid_location(map, v[1][0][0][0], v[1][0][0][1],
			      v[1][0][0][2]) &&
	  Rast3d_is_valid_location(map, v[1][0][1][0], v[1][0][1][1],
			      v[1][0][1][2]) &&
	  Rast3d_is_valid_location(map, v[1][1][0][0], v[1][1][0][1],
			      v[1][1][0][2]) &&
	  Rast3d_is_valid_location(map, v[1][1][1][0], v[1][1][1][1],
			      v[1][1][1][2])))
	Rast3d_fatal_error("verifyCubeVertices: volume vertex out of range");
    return 0;
}


/*---------------------------------------------------------------------------*/

static int verifyVolumeEdges(int nx, int ny, int nz)
{
    if ((nx <= 0) || (ny <= 0) || (nz <= 0))
	Rast3d_fatal_error("verifyCubeEdges: Volume edge out of range");
    return 0;
}

/*---------------------------------------------------------------------------*/

void
Rast3d_get_volume_a(void *map, double u[2][2][2][3], int nx, int ny, int nz,
	       void *volumeBuf, int type)
{
    typedef double doubleArray[3];

    doubleArray *u000, *u001, *u010, *u011;
    doubleArray *u100, *u101, *u110, *u111;
    double v00[3], v01[3], v10[3], v11[3];
    double v0[3], v1[3];
    double v[3];
    double r, rp, s, sp, t, tp;
    double dx, dy, dz;
    int x, y, z, nxp, nyp, nzp;
    double *doubleBuf;
    float *floatBuf;

    doubleBuf = (double *)volumeBuf;
    floatBuf = (float *)volumeBuf;

    verifyVolumeVertices(map, u);
    verifyVolumeEdges(nx, ny, nz);

    nxp = nx * 2 + 1;
    nyp = ny * 2 + 1;
    nzp = nz * 2 + 1;

    u000 = (doubleArray *) u[0][0][0];
    u001 = (doubleArray *) u[0][0][1];
    u010 = (doubleArray *) u[0][1][0];
    u011 = (doubleArray *) u[0][1][1];
    u100 = (doubleArray *) u[1][0][0];
    u101 = (doubleArray *) u[1][0][1];
    u110 = (doubleArray *) u[1][1][0];
    u111 = (doubleArray *) u[1][1][1];

    for (dz = 1; dz < nzp; dz += 2) {
	r = 1. - (rp = dz / nz / 2.);
	v00[0] = r * (*u000)[0] + rp * (*u100)[0];
	v00[1] = r * (*u000)[1] + rp * (*u100)[1];
	v00[2] = r * (*u000)[2] + rp * (*u100)[2];

	v01[0] = r * (*u001)[0] + rp * (*u101)[0];
	v01[1] = r * (*u001)[1] + rp * (*u101)[1];
	v01[2] = r * (*u001)[2] + rp * (*u101)[2];

	v10[0] = r * (*u010)[0] + rp * (*u110)[0];
	v10[1] = r * (*u010)[1] + rp * (*u110)[1];
	v10[2] = r * (*u010)[2] + rp * (*u110)[2];

	v11[0] = r * (*u011)[0] + rp * (*u111)[0];
	v11[1] = r * (*u011)[1] + rp * (*u111)[1];
	v11[2] = r * (*u011)[2] + rp * (*u111)[2];

	for (dy = 1; dy < nyp; dy += 2) {
	    s = 1. - (sp = dy / ny / 2.);
	    v0[0] = s * v00[0] + sp * v10[0];
	    v0[1] = s * v00[1] + sp * v10[1];
	    v0[2] = s * v00[2] + sp * v10[2];

	    v1[0] = s * v01[0] + sp * v11[0];
	    v1[1] = s * v01[1] + sp * v11[1];
	    v1[2] = s * v01[2] + sp * v11[2];

	    for (dx = 1; dx < nxp; dx += 2) {
		t = 1. - (tp = dx / nx / 2.);
		v[0] = t * v0[0] + tp * v1[0];
		v[1] = t * v0[1] + tp * v1[1];
		v[2] = t * v0[2] + tp * v1[2];

		Rast3d_location2coord2(map, v[0], v[1], v[2], &x, &y, &z);
		/* DEBUG
		   printf ("(%d %d %d) (%lf %lf %lf) (%d %d %d) %lf\n", 
		   (int) dx / 2, (int) dy / 2, (int) dz / 2,
		   v[0], v[1], v[2],
		   x, y, z, 
		   Rast3d_get_double_region (map, x, y, z));
		 */
		if (type == DCELL_TYPE)
		    *(doubleBuf + ((int)dz / 2) * nx * ny +
		      ((int)dy / 2) * nx + (int)dx / 2) =
Rast3d_get_double_region(map, x, y, z);
		else
		    *(floatBuf + ((int)dz / 2) * nx * ny +
		      ((int)dy / 2) * nx + (int)dx / 2) =
Rast3d_get_float_region(map, x, y, z);
	    }
	}
    }
}

/*---------------------------------------------------------------------------*/

void
Rast3d_get_volume(void *map,
	      double originNorth, double originWest, double originBottom,
	      double vxNorth, double vxWest, double vxBottom,
	      double vyNorth, double vyWest, double vyBottom,
	      double vzNorth, double vzWest, double vzBottom,
	      int nx, int ny, int nz, void *volumeBuf, int type)
{
    double u[2][2][2][3];

    u[0][0][0][0] = originNorth;
    u[0][0][0][1] = originWest;
    u[0][0][0][2] = originBottom;

    u[0][0][1][0] = vxNorth;
    u[0][0][1][1] = vxWest;
    u[0][0][1][2] = vxBottom;

    u[1][0][0][0] = vzNorth;
    u[1][0][0][1] = vzWest;
    u[1][0][0][2] = vzBottom;

    u[1][0][1][0] = (u[0][0][1][0] - u[0][0][0][0]) + u[1][0][0][0];
    u[1][0][1][1] = (u[0][0][1][1] - u[0][0][0][1]) + u[1][0][0][1];
    u[1][0][1][2] = (u[0][0][1][2] - u[0][0][0][2]) + u[1][0][0][2];

    u[0][1][0][0] = vyNorth;
    u[0][1][0][1] = vyWest;
    u[0][1][0][2] = vyBottom;

    u[0][1][1][0] = (u[0][0][1][0] - u[0][0][0][0]) + u[0][1][0][0];
    u[0][1][1][1] = (u[0][0][1][1] - u[0][0][0][1]) + u[0][1][0][1];
    u[0][1][1][2] = (u[0][0][1][2] - u[0][0][0][2]) + u[0][1][0][2];

    u[1][1][0][0] = (u[1][0][0][0] - u[0][0][0][0]) + u[0][1][0][0];
    u[1][1][0][1] = (u[1][0][0][1] - u[0][0][0][1]) + u[0][1][0][1];
    u[1][1][0][2] = (u[1][0][0][2] - u[0][0][0][2]) + u[0][1][0][2];

    u[1][1][1][0] = (u[1][0][0][0] - u[0][0][0][0]) + u[0][1][1][0];
    u[1][1][1][1] = (u[1][0][0][1] - u[0][0][0][1]) + u[0][1][1][1];
    u[1][1][1][2] = (u[1][0][0][2] - u[0][0][0][2]) + u[0][1][1][2];

    Rast3d_get_volume_a(map, u, nx, ny, nz, volumeBuf, type);
}

/*---------------------------------------------------------------------------*/

void
Rast3d_get_aligned_volume(void *map,
		     double originNorth, double originWest,
		     double originBottom, double lengthNorth,
		     double lengthWest, double lengthBottom, int nx, int ny,
		     int nz, void *volumeBuf, int type)
{
    Rast3d_get_volume(map,
		  originNorth, originWest, originBottom,
		  originNorth + lengthNorth, originWest, originBottom,
		  originNorth, originWest + lengthWest, originBottom,
		  originNorth, originWest, originBottom + lengthBottom,
		  nx, ny, nz, volumeBuf, type);
}

/*---------------------------------------------------------------------------*/

void
Rast3d_make_aligned_volume_file(void *map, const char *fileName,
			  double originNorth, double originWest,
			  double originBottom, double lengthNorth,
			  double lengthWest, double lengthBottom, int nx,
			  int ny, int nz)
{
    void *volumeBuf;
    void *mapVolume;
    int x, y, z, eltLength;
    RASTER3D_Region region;

    volumeBuf = Rast3d_malloc(nx * ny * nz * sizeof(Rast3d_get_file_type()));
    if (volumeBuf == NULL)
	Rast3d_fatal_error("Rast3d_make_aligned_volume_file: error in Rast3d_malloc");

    Rast3d_get_aligned_volume(map,
			 originNorth, originWest, originBottom,
			 lengthNorth, lengthWest, lengthBottom,
			 nx, ny, nz, volumeBuf, Rast3d_get_file_type());

    region.north = originNorth;
    region.south = originNorth + lengthNorth;
    region.east = originWest;
    region.west = originWest + lengthWest;
    region.top = originBottom;
    region.bottom = originBottom + lengthBottom;

    region.rows = ny;
    region.cols = nx;
    region.depths = nz;

    mapVolume = Rast3d_open_cell_new(fileName, Rast3d_get_file_type(),
				RASTER3D_USE_CACHE_DEFAULT, &region);
    if (mapVolume == NULL)
	Rast3d_fatal_error("Rast3d_make_aligned_volume_file: error in Rast3d_open_cell_new");

    eltLength = Rast3d_length(Rast3d_get_file_type());

    for (z = 0; z < nz; z++) {
	for (y = 0; y < ny; y++) {
	    for (x = 0; x < nx; x++) {
		/* Rast3d_putValueRegion? */
		if (!Rast3d_put_value(mapVolume, x, y, z,
				  G_incr_void_ptr(volumeBuf,
						  (z * ny * nx + y * nx +
						   x) * eltLength),
				  Rast3d_file_type_map(mapVolume)))
		    Rast3d_fatal_error
			("Rast3d_make_aligned_volume_file: error in Rast3d_put_value");
	    }
	}
    }

    if (!Rast3d_close(mapVolume))
	Rast3d_fatal_error("Rast3d_make_aligned_volume_file: error in Rast3d_close");

    Rast3d_free(volumeBuf);
}
