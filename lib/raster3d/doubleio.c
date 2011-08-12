#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

int Rast3d_write_doubles(int fd, int useXdr, const double *i, int nofNum)
{
    int firstTime = 1;
    XDR xdrEncodeStream;
    char xdrDoubleBuf[RASTER3D_XDR_DOUBLE_LENGTH * 1024];
    u_int n;

    if (nofNum <= 0)
	Rast3d_fatal_error("Rast3d_write_doubles: nofNum out of range");

    if (useXdr == RASTER3D_NO_XDR) {
	if (write(fd, i, sizeof(double) * nofNum) != sizeof(double) * nofNum) {
	    Rast3d_error("Rast3d_write_doubles: writing to file failed");
	    return 0;
	}
	else {
	    return 1;
	}
    }


    if (firstTime) {
	xdrmem_create(&xdrEncodeStream, xdrDoubleBuf,
		      RASTER3D_XDR_DOUBLE_LENGTH * 1024, XDR_ENCODE);
	firstTime = 1;
    }

    do {
	n = nofNum % 1024;
	if (n == 0)
	    n = 1024;

	if (!xdr_setpos(&xdrEncodeStream, 0)) {
	    Rast3d_error("Rast3d_write_doubles: positioning xdr failed");
	    return 0;
	}

	if (!xdr_vector(&xdrEncodeStream, (char *)i, n, sizeof(double),
			(xdrproc_t) xdr_double)) {
	    Rast3d_error("Rast3d_write_doubles: writing xdr failed");
	    return 0;
	}

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
    int firstTime = 1;
    XDR xdrDecodeStream;
    char xdrDoubleBuf[RASTER3D_XDR_DOUBLE_LENGTH * 1024];
    u_int n;

    if (nofNum <= 0)
	Rast3d_fatal_error("Rast3d_read_doubles: nofNum out of range");

    if (useXdr == RASTER3D_NO_XDR) {
	if (read(fd, i, sizeof(double) * nofNum) != sizeof(double) * nofNum) {
	    Rast3d_error("Rast3d_read_doubles: reading from file failed");
	    return 0;
	}
	else {
	    return 1;
	}
    }

    if (firstTime) {
	xdrmem_create(&xdrDecodeStream, xdrDoubleBuf,
		      RASTER3D_XDR_DOUBLE_LENGTH * 1024, XDR_DECODE);
	firstTime = 1;
    }

    do {
	n = nofNum % 1024;
	if (n == 0)
	    n = 1024;

	if (read(fd, xdrDoubleBuf, RASTER3D_XDR_DOUBLE_LENGTH * n) !=
	    RASTER3D_XDR_DOUBLE_LENGTH * n) {
	    Rast3d_error("Rast3d_read_doubles: reading xdr from file failed");
	    return 0;
	}

	if (!xdr_setpos(&xdrDecodeStream, 0)) {
	    Rast3d_error("Rast3d_read_doubles: positioning xdr failed");
	    return 0;
	}

	if (!xdr_vector(&xdrDecodeStream, (char *)i, n, sizeof(double),
			(xdrproc_t) xdr_double)) {
	    Rast3d_error("Rast3d_read_doubles: reading xdr failed");
	    return 0;
	}

	nofNum -= n;
	i += n;
    } while (nofNum);

    return 1;
}
