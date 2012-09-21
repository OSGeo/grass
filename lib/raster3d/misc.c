#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <grass/raster.h>

#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

int Rast3d_g3d_type2cell_type(int g3dType)
{
    if (g3dType == FCELL_TYPE)
	return FCELL_TYPE;
    return DCELL_TYPE;
}

/*---------------------------------------------------------------------------*/

void
Rast3d_copy_float2Double(const float *src, int offsSrc, double *dst, int offsDst,
		     int nElts)
{
    int i;

    src += offsSrc;
    dst += offsDst;

    for (i = 0; i < nElts; i++)
	dst[i] = (double)src[i];
}

/*---------------------------------------------------------------------------*/

void
Rast3d_copy_double2Float(const double *src, int offsSrc, float *dst, int offsDst,
		     int nElts)
{
    int i;

    src += offsSrc;
    dst += offsDst;

    for (i = 0; i < nElts; i++)
	dst[i] = (float)src[i];
}

/*---------------------------------------------------------------------------*/

void
Rast3d_copy_values(const void *src, int offsSrc, int typeSrc, void *dst,
	       int offsDst, int typeDst, int nElts)
{
    int eltLength;

    if ((typeSrc == FCELL_TYPE) && (typeDst == DCELL_TYPE)) {
	Rast3d_copy_float2Double(src, offsSrc, dst, offsDst, nElts);
	return;
    }

    if ((typeSrc == DCELL_TYPE) && (typeDst == FCELL_TYPE)) {
	Rast3d_copy_double2Float(src, offsSrc, dst, offsDst, nElts);
	return;
    }

    eltLength = Rast3d_length(typeSrc);

    src = G_incr_void_ptr(src, eltLength * offsSrc);
    dst = G_incr_void_ptr(dst, eltLength * offsDst);

    memcpy(dst, src, nElts * eltLength);
}

/*---------------------------------------------------------------------------*/

int Rast3d_length(int t)
{
    if (!RASTER3D_IS_CORRECT_TYPE(t))
	Rast3d_fatal_error("Rast3d_length: invalid type");

    if (t == FCELL_TYPE)
	return sizeof(FCELL);
    if (t == DCELL_TYPE)
	return sizeof(DCELL);
    return 0;
}

int Rast3d_extern_length(int t)
{
    if (!RASTER3D_IS_CORRECT_TYPE(t))
	Rast3d_fatal_error("Rast3d_extern_length: invalid type");

    if (t == FCELL_TYPE)
	return RASTER3D_XDR_FLOAT_LENGTH;
    if (t == DCELL_TYPE)
	return RASTER3D_XDR_DOUBLE_LENGTH;
    return 0;
}
