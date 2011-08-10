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

int G3d_isXdrNullNum(const void *num, int isFloat)
{
    static const char null_bytes[8] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    return memcmp(num, null_bytes, isFloat ? 4 : 8) == 0;
}

/*---------------------------------------------------------------------------*/

int G3d_isXdrNullFloat(const float *f)
{
    return G3d_isXdrNullNum(f, 1);
}

/*---------------------------------------------------------------------------*/

int G3d_isXdrNullDouble(const double *d)
{
    return G3d_isXdrNullNum(d, 0);
}

/*---------------------------------------------------------------------------*/

void G3d_setXdrNullNum(void *num, int isFloat)
{
    static const char null_bytes[8] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    memcpy(num, null_bytes, isFloat ? 4 : 8);
}

/*---------------------------------------------------------------------------*/

void G3d_setXdrNullDouble(double *d)
{
    G3d_setXdrNullNum(d, 0);
}

/*---------------------------------------------------------------------------*/

void G3d_setXdrNullFloat(float *f)
{
    G3d_setXdrNullNum(f, 1);
}

/*---------------------------------------------------------------------------*/

XDR xdrEncodeStream, xdrDecodeStream;	/* xdr support structures */

int G3d_initFpXdr(RASTER3D_Map * map, int misuseBytes)



 /* nof addtl bytes allocated for the xdr array so that */
		      /* the array can also be (mis)used for other purposes */
{
    int doAlloc;

    doAlloc = 0;

    if (xdr == NULL) {
	xdrLength = map->tileSize * RASTER3D_MAX(map->numLengthExtern,
					    map->numLengthIntern) +
	    misuseBytes;
	xdr = G3d_malloc(xdrLength);
	if (xdr == NULL) {
	    G3d_error("G3d_initFpXdr: error in G3d_malloc");
	    return 0;
	}

	doAlloc = 1;
    }
    else if (map->tileSize * RASTER3D_MAX(map->numLengthExtern,
				     map->numLengthIntern) + misuseBytes
	     > xdrLength) {
	xdrLength = map->tileSize * RASTER3D_MAX(map->numLengthExtern,
					    map->numLengthIntern) +
	    misuseBytes;
	xdr = G3d_realloc(xdr, xdrLength);
	if (xdr == NULL) {
	    G3d_error("G3d_initFpXdr: error in G3d_realloc");
	    return 0;
	}

	doAlloc = 1;
    }

    if (doAlloc) {
	xdrmem_create(&(xdrEncodeStream), xdr, (u_int) xdrLength, XDR_ENCODE);
	xdrmem_create(&(xdrDecodeStream), xdr, (u_int) xdrLength, XDR_DECODE);
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

static void *xdrTmp;
static int dstType, srcType, type, externLength, eltLength, isFloat, useXdr;
static int (*xdrFun) ();
static XDR *xdrs;
static double tmpValue, *tmp;

int G3d_initCopyToXdr(RASTER3D_Map * map, int sType)
{
    xdrTmp = xdr;
    useXdr = map->useXdr;
    srcType = sType;

    if (map->useXdr == RASTER3D_USE_XDR) {
	if (!xdr_setpos(&(xdrEncodeStream), 0)) {
	    G3d_error("G3d_InitCopyToXdr: positioning xdr failed");
	    return 0;
	}
	xdrs = &(xdrEncodeStream);
    }

    type = map->type;
    isFloat = (type == FCELL_TYPE);
    externLength = G3d_externLength(type);
    eltLength = G3d_length(srcType);
    if (isFloat)
	xdrFun = xdr_float;
    else
	xdrFun = xdr_double;
    tmp = &tmpValue;

    return 1;
}

/*---------------------------------------------------------------------------*/

int G3d_copyToXdr(const void *src, int nofNum)
{
    int i;

    if (useXdr == RASTER3D_NO_XDR) {
	G3d_copyValues(src, 0, srcType, xdrTmp, 0, type, nofNum);
	xdrTmp = G_incr_void_ptr(xdrTmp, nofNum * G3d_externLength(type));
	return 1;
    }

    for (i = 0; i < nofNum; i++, src = G_incr_void_ptr(src, eltLength)) {

	if (G3d_isNullValueNum(src, srcType)) {
	    G3d_setXdrNullNum(xdrTmp, isFloat);
	    if (!xdr_setpos(xdrs, xdr_getpos(xdrs) + externLength)) {
		G3d_error("G3d_copyToXdr: positioning xdr failed");
		return 0;
	    }
	}
	else {
	    if (type == srcType) {
		if (xdrFun(xdrs, src) < 0) {
		    G3d_error("G3d_copyToXdr: writing xdr failed");
		    return 0;
		}
	    }
	    else {
		if (type == FCELL_TYPE)
		    *((float *)tmp) = (float)*((double *)src);
		else
		    *((double *)tmp) = (double)*((float *)src);
		if (xdrFun(xdrs, tmp) < 0) {
		    G3d_error("G3d_copyToXdr: writing xdr failed");
		    return 0;
		}
	    }
	}

	xdrTmp = G_incr_void_ptr(xdrTmp, externLength);
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

int G3d_initCopyFromXdr(RASTER3D_Map * map, int dType)
{
    xdrTmp = xdr;
    useXdr = map->useXdr;
    dstType = dType;

    if (useXdr == RASTER3D_USE_XDR) {
	if (!xdr_setpos(&(xdrDecodeStream), 0)) {
	    G3d_error("G3d_initCopyFromXdr: positioning xdr failed");
	    return 0;
	}
	xdrs = &(xdrDecodeStream);
    }

    type = map->type;
    isFloat = (type == FCELL_TYPE);
    externLength = G3d_externLength(type);
    eltLength = G3d_length(dstType);
    if (isFloat)
	xdrFun = xdr_float;
    else
	xdrFun = xdr_double;
    tmp = &tmpValue;

    return 1;
}

/*---------------------------------------------------------------------------*/

int G3d_copyFromXdr(int nofNum, void *dst)
{
    int i;

    if (useXdr == RASTER3D_NO_XDR) {
	G3d_copyValues(xdrTmp, 0, type, dst, 0, dstType, nofNum);
	xdrTmp = G_incr_void_ptr(xdrTmp, nofNum * G3d_externLength(type));
	return 1;
    }

    for (i = 0; i < nofNum; i++, dst = G_incr_void_ptr(dst, eltLength)) {

	if (G3d_isXdrNullNum(xdrTmp, isFloat)) {
	    G3d_setNullValue(dst, 1, dstType);
	    if (!xdr_setpos(xdrs, xdr_getpos(xdrs) + externLength)) {
		G3d_error("G3d_copyFromXdr: positioning xdr failed");
		return 0;
	    }
	}
	else {
	    if (type == dstType) {
		if (xdrFun(xdrs, dst) < 0) {
		    G3d_error("G3d_copyFromXdr: reading xdr failed");
		    return 0;
		}
	    }
	    else {
		if (xdrFun(xdrs, tmp) < 0) {
		    G3d_error("G3d_copyFromXdr: reading xdr failed");
		    return 0;
		}
		if (type == FCELL_TYPE)
		    *((double *)dst) = (double)*((float *)tmp);
		else
		    *((float *)dst) = (float)*((double *)tmp);
	    }
	}

	xdrTmp = G_incr_void_ptr(xdrTmp, externLength);
    }

    return 1;
}
