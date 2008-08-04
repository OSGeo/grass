#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/

int G3d_writeDoubles(int fd, int useXdr, const double *i, int nofNum)
{
    int firstTime = 1;
    XDR xdrEncodeStream;
    char xdrDoubleBuf[G3D_XDR_DOUBLE_LENGTH * 1024];
    u_int n;

    if (nofNum <= 0)
	G3d_fatalError("G3d_writeDoubles: nofNum out of range");

    if (useXdr == G3D_NO_XDR) {
	if (write(fd, i, sizeof(double) * nofNum) != sizeof(double) * nofNum) {
	    G3d_error("G3d_writeDoubles: writing to file failed");
	    return 0;
	}
	else {
	    return 1;
	}
    }


    if (firstTime) {
	xdrmem_create(&xdrEncodeStream, xdrDoubleBuf,
		      G3D_XDR_DOUBLE_LENGTH * 1024, XDR_ENCODE);
	firstTime = 1;
    }

    do {
	n = nofNum % 1024;
	if (n == 0)
	    n = 1024;

	if (!xdr_setpos(&xdrEncodeStream, 0)) {
	    G3d_error("G3d_writeDoubles: positioning xdr failed");
	    return 0;
	}

	if (!xdr_vector(&xdrEncodeStream, (char *)i, n, sizeof(double),
			(xdrproc_t) xdr_double)) {
	    G3d_error("G3d_writeDoubles: writing xdr failed");
	    return 0;
	}

	if (write(fd, xdrDoubleBuf, G3D_XDR_DOUBLE_LENGTH * n) !=
	    G3D_XDR_DOUBLE_LENGTH * n) {
	    G3d_error("G3d_writeDoubles: writing xdr to file failed");
	    return 0;
	}

	nofNum -= n;
	i += n;
    } while (nofNum);

    return 1;
}

/*---------------------------------------------------------------------------*/

int G3d_readDoubles(int fd, int useXdr, double *i, int nofNum)
{
    int firstTime = 1;
    XDR xdrDecodeStream;
    char xdrDoubleBuf[G3D_XDR_DOUBLE_LENGTH * 1024];
    u_int n;

    if (nofNum <= 0)
	G3d_fatalError("G3d_readDoubles: nofNum out of range");

    if (useXdr == G3D_NO_XDR) {
	if (read(fd, i, sizeof(double) * nofNum) != sizeof(double) * nofNum) {
	    G3d_error("G3d_readDoubles: reading from file failed");
	    return 0;
	}
	else {
	    return 1;
	}
    }

    if (firstTime) {
	xdrmem_create(&xdrDecodeStream, xdrDoubleBuf,
		      G3D_XDR_DOUBLE_LENGTH * 1024, XDR_DECODE);
	firstTime = 1;
    }

    do {
	n = nofNum % 1024;
	if (n == 0)
	    n = 1024;

	if (read(fd, xdrDoubleBuf, G3D_XDR_DOUBLE_LENGTH * n) !=
	    G3D_XDR_DOUBLE_LENGTH * n) {
	    G3d_error("G3d_readDoubles: reading xdr from file failed");
	    return 0;
	}

	if (!xdr_setpos(&xdrDecodeStream, 0)) {
	    G3d_error("G3d_readDoubles: positioning xdr failed");
	    return 0;
	}

	if (!xdr_vector(&xdrDecodeStream, (char *)i, n, sizeof(double),
			(xdrproc_t) xdr_double)) {
	    G3d_error("G3d_readDoubles: reading xdr failed");
	    return 0;
	}

	nofNum -= n;
	i += n;
    } while (nofNum);

    return 1;
}
