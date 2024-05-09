#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

int Rast3d_long_encode(long *source, unsigned char *dst, int nofNums)
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

void Rast3d_long_decode(unsigned char *source, long *dst, int nofNums,
                        int longNbytes)
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
<<<<<<< HEAD
        if ((eltLength >= (int)RASTER3D_LONG_LENGTH) && (*dest != 0))
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        if ((eltLength >= (int)RASTER3D_LONG_LENGTH) && (*dest != 0))
=======
        if ((eltLength >= RASTER3D_LONG_LENGTH) && (*dest != 0))
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        if ((eltLength >= RASTER3D_LONG_LENGTH) && (*dest != 0))
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        if ((eltLength >= (int)RASTER3D_LONG_LENGTH) && (*dest != 0))
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
        if ((eltLength >= RASTER3D_LONG_LENGTH) && (*dest != 0))
=======
        if ((eltLength >= (int)RASTER3D_LONG_LENGTH) && (*dest != 0))
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
            Rast3d_fatal_error("Rast3d_long_decode: decoded long too long");
        dest--;
    }

    while (eltLength--) {
        srcStop = source - nofNums;
        dest = dst;
        dest += nofNums - 1;
        while (source != srcStop) {
            *dest *= 256;
            *dest += *source--;
<<<<<<< HEAD
            if ((eltLength >= (int)RASTER3D_LONG_LENGTH) && (*dest != 0))
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            if ((eltLength >= (int)RASTER3D_LONG_LENGTH) && (*dest != 0))
=======
            if ((eltLength >= RASTER3D_LONG_LENGTH) && (*dest != 0))
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            if ((eltLength >= RASTER3D_LONG_LENGTH) && (*dest != 0))
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            if ((eltLength >= (int)RASTER3D_LONG_LENGTH) && (*dest != 0))
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
            if ((eltLength >= RASTER3D_LONG_LENGTH) && (*dest != 0))
=======
            if ((eltLength >= (int)RASTER3D_LONG_LENGTH) && (*dest != 0))
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
>>>>>>> osgeo-main
                Rast3d_fatal_error("Rast3d_long_decode: decoded long too long");
            dest--;
        }
    }
}
