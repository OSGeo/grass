#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "raster3d_intern.h"

/*---------------------------------------------------------------------------*/

int G3d_writeInts(int fd, int useXdr, const int *i, int nofNum)
{
    int firstTime = 1;
    XDR xdrEncodeStream;
    char xdrIntBuf[RASTER3D_XDR_INT_LENGTH * 1024];
    u_int n;

    if (nofNum <= 0)
	G3d_fatalError("G3d_writeInts: nofNum out of range");

    if (useXdr == RASTER3D_NO_XDR) {
	if (write(fd, i, sizeof(int) * nofNum) != sizeof(int) * nofNum) {
	    G3d_error("G3d_writeInts: writing to file failed");
	    return 0;
	}
	else {
	    return 1;
	}
    }

    if (firstTime) {
	xdrmem_create(&xdrEncodeStream, xdrIntBuf, RASTER3D_XDR_INT_LENGTH * 1024,
		      XDR_ENCODE);
	firstTime = 1;
    }

    do {
	n = nofNum % 1024;
	if (n == 0)
	    n = 1024;

	if (!xdr_setpos(&xdrEncodeStream, 0)) {
	    G3d_error("G3d_writeInts: positioning xdr failed");
	    return 0;
	}

	if (!xdr_vector(&xdrEncodeStream, (char *)i, n, sizeof(int),
			(xdrproc_t) xdr_int)) {
	    G3d_error("G3d_writeInts: writing xdr failed");
	    return 0;
	}

	if (write(fd, xdrIntBuf, RASTER3D_XDR_INT_LENGTH * n) !=
	    RASTER3D_XDR_INT_LENGTH * n) {
	    G3d_error("G3d_writeInts: writing xdr to file failed");
	    return 0;
	}

	nofNum -= n;
	i += n;
    } while (nofNum);

    return 1;
}

/*---------------------------------------------------------------------------*/

int G3d_readInts(int fd, int useXdr, int *i, int nofNum)
{
    int firstTime = 1;
    XDR xdrDecodeStream;
    char xdrIntBuf[RASTER3D_XDR_INT_LENGTH * 1024];
    u_int n;

    if (nofNum <= 0)
	G3d_fatalError("G3d_readInts: nofNum out of range");

    if (useXdr == RASTER3D_NO_XDR) {
	if (read(fd, i, sizeof(int) * nofNum) != sizeof(int) * nofNum) {
	    G3d_error("G3d_readInts: reading from file failed");
	    return 0;
	}
	else {
	    return 1;
	}
    }

    if (firstTime) {
	xdrmem_create(&xdrDecodeStream, xdrIntBuf, RASTER3D_XDR_INT_LENGTH * 1024,
		      XDR_DECODE);
	firstTime = 1;
    }

    do {
	n = nofNum % 1024;
	if (n == 0)
	    n = 1024;

	if (read(fd, xdrIntBuf, RASTER3D_XDR_INT_LENGTH * n) !=
	    RASTER3D_XDR_INT_LENGTH * n) {
	    G3d_error("G3d_readInts: reading xdr from file failed");
	    return 0;
	}

	if (!xdr_setpos(&xdrDecodeStream, 0)) {
	    G3d_error("G3d_readInts: positioning xdr failed");
	    return 0;
	}

	if (!xdr_vector(&xdrDecodeStream, (char *)i, n, sizeof(int),
			(xdrproc_t) xdr_int)) {
	    G3d_error("G3d_readInts: reading xdr failed");
	    return 0;
	}

	nofNum -= n;
	i += n;
    } while (nofNum);

    return 1;
}
