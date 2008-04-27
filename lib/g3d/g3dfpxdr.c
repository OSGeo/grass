#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/

#define XDR_NULL_BYTE_0 255
#define XDR_NULL_BYTE_1 255
#define XDR_NULL_BYTE_2 255
#define XDR_NULL_BYTE_3 255
#define XDR_NULL_BYTE_4 255
#define XDR_NULL_BYTE_5 255
#define XDR_NULL_BYTE_6 255
#define XDR_NULL_BYTE_7 255

int
G3d_isXdrNullNum  (unsigned char *num, int isFloat)

{
  if (*num++ != XDR_NULL_BYTE_0) return 0;
  if (*num++ != XDR_NULL_BYTE_1) return 0;
  if (*num++ != XDR_NULL_BYTE_2) return 0;
  if (*num++ != XDR_NULL_BYTE_3) return 0;

  if (isFloat) return 1;

  if (*num++ != XDR_NULL_BYTE_4) return 0;
  if (*num++ != XDR_NULL_BYTE_5) return 0;
  if (*num++ != XDR_NULL_BYTE_6) return 0;
  return (*num == XDR_NULL_BYTE_7);
}

/*---------------------------------------------------------------------------*/

int
G3d_isXdrNullFloat  (float *f)

{
  return G3d_isXdrNullNum ((unsigned char *) f, 1);
}

/*---------------------------------------------------------------------------*/

int
G3d_isXdrNullDouble  (double *d)

{
  return G3d_isXdrNullNum ((unsigned char *) d, 0);
}

/*---------------------------------------------------------------------------*/

void
G3d_setXdrNullNum  (unsigned char *num, int isFloat)

{
  *num++ = XDR_NULL_BYTE_0;
  *num++ = XDR_NULL_BYTE_1;
  *num++ = XDR_NULL_BYTE_2;
  *num++ = XDR_NULL_BYTE_3;

  if (isFloat) return;	
 
  *num++ = XDR_NULL_BYTE_4;
  *num++ = XDR_NULL_BYTE_5;
  *num++ = XDR_NULL_BYTE_6;
  *num = XDR_NULL_BYTE_7;
}
  
/*---------------------------------------------------------------------------*/

void
G3d_setXdrNullDouble  (double *d)

{
  G3d_setXdrNullNum ((unsigned char *) d, 0);
}

/*---------------------------------------------------------------------------*/

void
G3d_setXdrNullFloat  (float *f)

{
  G3d_setXdrNullNum ((unsigned char *) f, 1);
}

/*---------------------------------------------------------------------------*/

XDR xdrEncodeStream, xdrDecodeStream; /* xdr support structures */

int
G3d_initFpXdr  (G3D_Map *map, int misuseBytes)



 /* nof addtl bytes allocated for the xdr array so that */
                      /* the array can also be (mis)used for other purposes */

{
  int doAlloc;

  doAlloc = 0;

  if (xdr == NULL) {
    xdrLength = map->tileSize * G3D_MAX (map->numLengthExtern,
					 map->numLengthIntern) + misuseBytes;
    xdr = G3d_malloc (xdrLength);
    if (xdr == NULL) {
      G3d_error ("G3d_initFpXdr: error in G3d_malloc");
      return 0;
    }
     
    doAlloc = 1;
  } else 
    if (map->tileSize * G3D_MAX (map->numLengthExtern,
				 map->numLengthIntern) + misuseBytes
	> xdrLength){
      xdrLength = map->tileSize * G3D_MAX (map->numLengthExtern,
					   map->numLengthIntern) + misuseBytes;
      xdr = G3d_realloc (xdr, xdrLength);
      if (xdr == NULL) {
	G3d_error ("G3d_initFpXdr: error in G3d_realloc");
	return 0;
      }

      doAlloc = 1;
    }

  if (doAlloc) {
    xdrmem_create (&(xdrEncodeStream), xdr, (u_int) xdrLength, 
		   XDR_ENCODE);
    xdrmem_create (&(xdrDecodeStream), xdr, (u_int) xdrLength,
		   XDR_DECODE);
  }

  return 1;
}

/*---------------------------------------------------------------------------*/

char *xdrTmp;
static int dstType, srcType, type, externLength, eltLength, isFloat, useXdr;
static int (* xdrFun) ();
static XDR *xdrs;
static double tmpValue, *tmp;

int
G3d_initCopyToXdr  (G3D_Map *map, int sType)

{
  xdrTmp = xdr;
  useXdr = map->useXdr;
  srcType = sType;

  if (map->useXdr == G3D_USE_XDR) {
    if (! xdr_setpos (&(xdrEncodeStream), 0)) {
      G3d_error ("G3d_InitCopyToXdr: positioning xdr failed");
      return 0;
    }
    xdrs = &(xdrEncodeStream);
  }

  type = map->type;
  isFloat = (type == FCELL_TYPE);
  externLength = G3d_externLength (type);
  eltLength = G3d_length (srcType);
  if (isFloat) xdrFun = xdr_float; else xdrFun = xdr_double;
  tmp = &tmpValue;

  return 1;
}

/*---------------------------------------------------------------------------*/

int
G3d_copyToXdr  (char *src, int nofNum)

{
  char *srcLast;

  if (useXdr == G3D_NO_XDR) {
    G3d_copyValues (src, 0, srcType, xdrTmp, 0, type, nofNum);
    xdrTmp += nofNum * G3d_externLength (type);
    return 1;
  }

  srcLast = src + nofNum * eltLength;

  for ( ; src != srcLast ; src += eltLength) {

    if (G3d_isNullValueNum (src, srcType)) {
      G3d_setXdrNullNum ((unsigned char *) xdrTmp, isFloat);
      if (! xdr_setpos (xdrs, xdr_getpos (xdrs) + externLength)) {
	G3d_error ("G3d_copyToXdr: positioning xdr failed");
      return 0;
    }
    } else {
      if (type == srcType) {
	if (xdrFun (xdrs, src) < 0) {
	  G3d_error ("G3d_copyToXdr: writing xdr failed");
	  return 0;
	}
      } else {
	if (type == FCELL_TYPE)
	  *((float *) tmp) = (float) *((double *) src);
	else
	  *((double *) tmp) = (double) *((float *) src);
	if (xdrFun (xdrs, tmp) < 0) {
	  G3d_error ("G3d_copyToXdr: writing xdr failed");
	  return 0;
	}
      }
    }

    xdrTmp += externLength;
  }

  return 1;
}

/*---------------------------------------------------------------------------*/

int
G3d_initCopyFromXdr  (G3D_Map *map, int dType)

{
  xdrTmp = xdr;
  useXdr = map->useXdr;
  dstType = dType;

  if (useXdr == G3D_USE_XDR) {
    if (! xdr_setpos (&(xdrDecodeStream), 0)) {
      G3d_error ("G3d_initCopyFromXdr: positioning xdr failed");
      return 0;
    }
    xdrs = &(xdrDecodeStream);
  }

  type = map->type;
  isFloat = (type == FCELL_TYPE);
  externLength = G3d_externLength (type);
  eltLength = G3d_length (dstType);
  if (isFloat) xdrFun = xdr_float; else xdrFun = xdr_double;
  tmp = &tmpValue;

  return 1;
}

/*---------------------------------------------------------------------------*/

int
G3d_copyFromXdr  (int nofNum, char *dst)

{
  char *dstLast;

  if (useXdr == G3D_NO_XDR) {
    G3d_copyValues (xdrTmp, 0, type, dst, 0, dstType, nofNum);
    xdrTmp += nofNum * G3d_externLength (type);
    return 1;
  }

  dstLast = dst + nofNum * eltLength;

  for ( ; dst != dstLast ; dst += eltLength) {

    if (G3d_isXdrNullNum ((unsigned char *) xdrTmp, isFloat)) {
      G3d_setNullValue (dst, 1, dstType);
      if (! xdr_setpos (xdrs, xdr_getpos (xdrs) + externLength)) {
	G3d_error ("G3d_copyFromXdr: positioning xdr failed");
	return 0;
      }
    } else {
      if (type == dstType) {
	if (xdrFun (xdrs, dst) < 0) {
	  G3d_error ("G3d_copyFromXdr: reading xdr failed");
	  return 0;
	}
      } else {
	if (xdrFun (xdrs, tmp) < 0) {
	  G3d_error ("G3d_copyFromXdr: reading xdr failed");
	  return 0;
	}
	if (type == FCELL_TYPE)
	  *((double *) dst) = (double) *((float *) tmp);
	else
	  *((float *) dst) = (float) *((double *) tmp);
      }
    }

    xdrTmp += externLength;
  }

  return 1;
}
