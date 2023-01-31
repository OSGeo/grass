#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

int Rast3d_write_doubles(int fd, int useXdr, const double *i, int nofNum)
{
    char xdrDoubleBuf[RASTER3D_XDR_DOUBLE_LENGTH * 1024];
    unsigned int n;

    if (nofNum <= 0)
        Rast3d_fatal_error("Rast3d_write_doubles: nofNum out of range");

    if (useXdr == RASTER3D_NO_XDR) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        if (write(fd, i, sizeof(double) * nofNum) !=
            (int)sizeof(double) * nofNum) {
=======
        if (write(fd, i, sizeof(double) * nofNum) != sizeof(double) * nofNum) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        if (write(fd, i, sizeof(double) * nofNum) != sizeof(double) * nofNum) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        if (write(fd, i, sizeof(double) * nofNum) !=
            (int)sizeof(double) * nofNum) {
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
        if (write(fd, i, sizeof(double) * nofNum) != sizeof(double) * nofNum) {
=======
        if (write(fd, i, sizeof(double) * nofNum) !=
            (int)sizeof(double) * nofNum) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            Rast3d_error("Rast3d_write_doubles: writing to file failed");
            return 0;
        }
        else {
            return 1;
        }
    }

    do {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        unsigned int j;
=======
        int j;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        int j;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        unsigned int j;
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
        int j;
=======
        unsigned int j;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

        n = nofNum % 1024;
        if (n == 0)
            n = 1024;

        for (j = 0; j < n; j++)
            G_xdr_put_double(&xdrDoubleBuf[RASTER3D_XDR_DOUBLE_LENGTH * j], i);

        if (write(fd, xdrDoubleBuf, RASTER3D_XDR_DOUBLE_LENGTH * n) !=
            RASTER3D_XDR_DOUBLE_LENGTH * n) {
            Rast3d_error("Rast3d_write_doubles: writing xdr to file failed");
            return 0;
        }

        nofNum -= n;
        i += n;
    } while (nofNum);

    return 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d_read_doubles(int fd, int useXdr, double *i, int nofNum)
{
    char xdrDoubleBuf[RASTER3D_XDR_DOUBLE_LENGTH * 1024];
    unsigned int n;

    if (nofNum <= 0)
        Rast3d_fatal_error("Rast3d_read_doubles: nofNum out of range");

    if (useXdr == RASTER3D_NO_XDR) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        if (read(fd, i, sizeof(double) * nofNum) !=
            (int)sizeof(double) * nofNum) {
=======
        if (read(fd, i, sizeof(double) * nofNum) != sizeof(double) * nofNum) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        if (read(fd, i, sizeof(double) * nofNum) != sizeof(double) * nofNum) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        if (read(fd, i, sizeof(double) * nofNum) !=
            (int)sizeof(double) * nofNum) {
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
        if (read(fd, i, sizeof(double) * nofNum) != sizeof(double) * nofNum) {
=======
        if (read(fd, i, sizeof(double) * nofNum) !=
            (int)sizeof(double) * nofNum) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            Rast3d_error("Rast3d_read_doubles: reading from file failed");
            return 0;
        }
        else {
            return 1;
        }
    }

    do {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        unsigned int j;
=======
        int j;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        int j;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        unsigned int j;
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
        int j;
=======
        unsigned int j;
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

        n = nofNum % 1024;
        if (n == 0)
            n = 1024;

        if (read(fd, xdrDoubleBuf, RASTER3D_XDR_DOUBLE_LENGTH * n) !=
            RASTER3D_XDR_DOUBLE_LENGTH * n) {
            Rast3d_error("Rast3d_read_doubles: reading xdr from file failed");
            return 0;
        }

        for (j = 0; j < n; j++)
            G_xdr_get_double(i, &xdrDoubleBuf[RASTER3D_XDR_DOUBLE_LENGTH * j]);

        nofNum -= n;
        i += n;
    } while (nofNum);

    return 1;
}
