#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

int Rast3d_write_ints(int fd, int useXdr, const int *i, int nofNum)
{
    char xdrIntBuf[RASTER3D_XDR_INT_LENGTH * 1024];
    unsigned int n;

    if (nofNum <= 0)
	Rast3d_fatal_error("Rast3d_write_ints: nofNum out of range");

    if (useXdr == RASTER3D_NO_XDR) {
	if (write(fd, i, sizeof(int) * nofNum) != sizeof(int) * nofNum) {
	    Rast3d_error("Rast3d_write_ints: writing to file failed");
	    return 0;
	}
	else {
	    return 1;
	}
    }

    do {
	int j;
	n = nofNum % 1024;
	if (n == 0)
	    n = 1024;

	for (j = 0; j < n; j++)
	    G_xdr_put_int(&xdrIntBuf[RASTER3D_XDR_INT_LENGTH * j], i);

	if (write(fd, xdrIntBuf, RASTER3D_XDR_INT_LENGTH * n) !=
	    RASTER3D_XDR_INT_LENGTH * n) {
	    Rast3d_error("Rast3d_write_ints: writing xdr to file failed");
	    return 0;
	}

	nofNum -= n;
	i += n;
    } while (nofNum);

    return 1;
}

/*---------------------------------------------------------------------------*/

int Rast3d_read_ints(int fd, int useXdr, int *i, int nofNum)
{
    char xdrIntBuf[RASTER3D_XDR_INT_LENGTH * 1024];
    unsigned int n;

    if (nofNum <= 0)
	Rast3d_fatal_error("Rast3d_read_ints: nofNum out of range");

    if (useXdr == RASTER3D_NO_XDR) {
	if (read(fd, i, sizeof(int) * nofNum) != sizeof(int) * nofNum) {
	    Rast3d_error("Rast3d_read_ints: reading from file failed");
	    return 0;
	}
	else {
	    return 1;
	}
    }

    do {
	int j;
	n = nofNum % 1024;
	if (n == 0)
	    n = 1024;

	if (read(fd, xdrIntBuf, RASTER3D_XDR_INT_LENGTH * n) !=
	    RASTER3D_XDR_INT_LENGTH * n) {
	    Rast3d_error("Rast3d_read_ints: reading xdr from file failed");
	    return 0;
	}

	for (j = 0; j < n; j++)
	    G_xdr_get_int(i, &xdrIntBuf[RASTER3D_XDR_INT_LENGTH * j]);

	nofNum -= n;
	i += n;
    } while (nofNum);

    return 1;
}
