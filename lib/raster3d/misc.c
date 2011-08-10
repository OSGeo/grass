#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpc/types.h>
#include <rpc/xdr.h>

#include <grass/raster.h>

#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

int G3d_g3dType2cellType(int g3dType)
{
    if (g3dType == FCELL_TYPE)
	return FCELL_TYPE;
    return DCELL_TYPE;
}

/*---------------------------------------------------------------------------*/

void
G3d_copyFloat2Double(const float *src, int offsSrc, double *dst, int offsDst,
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
G3d_copyDouble2Float(const double *src, int offsSrc, float *dst, int offsDst,
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
G3d_copyValues(const void *src, int offsSrc, int typeSrc, void *dst,
	       int offsDst, int typeDst, int nElts)
{
    int eltLength;

    if ((typeSrc == FCELL_TYPE) && (typeDst == DCELL_TYPE)) {
	G3d_copyFloat2Double(src, offsSrc, dst, offsDst, nElts);
	return;
    }

    if ((typeSrc == DCELL_TYPE) && (typeDst == FCELL_TYPE)) {
	G3d_copyDouble2Float(src, offsSrc, dst, offsDst, nElts);
	return;
    }

    eltLength = G3d_length(typeSrc);

    src = G_incr_void_ptr(src, eltLength * offsSrc);
    dst = G_incr_void_ptr(dst, eltLength * offsDst);

    memcpy(dst, src, nElts * eltLength);
}

/*---------------------------------------------------------------------------*/

int G3d_length(int t)
{
    if (!G3D_IS_CORRECT_TYPE(t))
	G3d_fatalError("G3d_length: invalid type");

    if (t == FCELL_TYPE)
	return sizeof(FCELL);
    if (t == DCELL_TYPE)
	return sizeof(DCELL);
    return 0;
}

int G3d_externLength(int t)
{
    if (!G3D_IS_CORRECT_TYPE(t))
	G3d_fatalError("G3d_externLength: invalid type");

    if (t == FCELL_TYPE)
	return G3D_XDR_FLOAT_LENGTH;
    if (t == DCELL_TYPE)
	return G3D_XDR_DOUBLE_LENGTH;
    return 0;
}
