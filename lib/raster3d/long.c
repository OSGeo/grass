#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

int Rast3d_longEncode(long *source, unsigned char *dst, int nofNums)
{
    long *src, d;
    int eltLength, nBytes;
    unsigned char *dstStop, tmp;

    eltLength = RASTER3D_LONG_LENGTH;
    nBytes = 8;

    d = 1;

    while (eltLength--) {
	dstStop = dst + nofNums;
	src = source;

	while (dst != dstStop) {
	    tmp = ((*src++ / d) % 256);
	    if (tmp != 0)
		nBytes = RASTER3D_MIN(nBytes, eltLength);
	    *dst++ = tmp;
	}

	d *= 256;
    }

    return RASTER3D_LONG_LENGTH - nBytes;
}

/*---------------------------------------------------------------------------*/

void
Rast3d_longDecode(unsigned char *source, long *dst, int nofNums, int longNbytes)
{
    long *dest;
    int eltLength;
    unsigned char *srcStop;

    eltLength = longNbytes;

    source += nofNums * eltLength - 1;

    eltLength--;
    srcStop = source - nofNums;
    dest = dst;
    dest += nofNums - 1;
    while (source != srcStop) {
	*dest = *source--;
	if ((eltLength >= RASTER3D_LONG_LENGTH) && (*dest != 0))
	    Rast3d_fatalError("Rast3d_longDecode: decoded long too long");
	dest--;
    }

    while (eltLength--) {
	srcStop = source - nofNums;
	dest = dst;
	dest += nofNums - 1;
	while (source != srcStop) {
	    *dest *= 256;
	    *dest += *source--;
	    if ((eltLength >= RASTER3D_LONG_LENGTH) && (*dest != 0))
		Rast3d_fatalError("Rast3d_longDecode: decoded long too long");
	    dest--;
	}
    }
}
