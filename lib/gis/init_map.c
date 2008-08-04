
/*--------------------------------------------------------------------------*/

/* inititializes the random file with descriptor "fd" with "nofRows" rows
   of zero values columns. each row consists of "nofCols" columns. 
   assumes that the file is rewound and empty.

   returns 1 if successful and 0 for any kind of error. */

#include "G.h"

/*--------------------------------------------------------------------------*/

int G__random_d_initialize_0(int fd, int nofRows, int nofCols)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    int row, col;
    double zeroVal, *zeroValP;
    register XDR *xdrs;

    xdrs = &fcb->xdrstream;	/* xdr stream is initialized to write into */
    xdr_setpos(xdrs, 0);	/* G__.work_buf in 'opencell.c' */

    zeroVal = 0;
    zeroValP = &zeroVal;

    for (col = nofCols; col--;)
	if (!xdr_double(xdrs, zeroValP)) {
	    G_warning
		("G_random_d_initialize_0: xdr_double failed for index %d.\n",
		 col);
	    return -1;
	}

    for (row = 0; row < nofRows; row++)
	if (G__write_data(fd, row, nofCols) == -1) {
	    G_warning("G_random_d_initialize_0: write failed in row %d.\n",
		      row);
	    return -1;
	}

    return 1;
}

/*--------------------------------------------------------------------------*/

/* inititializes the random file with descriptor "fd" with "nofRows" rows
   of zero values columns. each row consists of "nofCols" columns. 
   assumes that the file is rewound and empty.

   returns 1 if successful and 0 for any kind of error. */


int G__random_f_initialize_0(int fd, int nofRows, int nofCols)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    int row, col;
    float zeroVal, *zeroValP;
    register XDR *xdrs;


    xdrs = &fcb->xdrstream;	/* xdr stream is initialized to write into */
    xdr_setpos(xdrs, 0);	/* G__.work_buf in 'opencell.c' */

    zeroVal = 0;
    zeroValP = &zeroVal;

    for (col = nofCols; col--;)
	if (!xdr_float(xdrs, zeroValP)) {
	    G_warning
		("G_random_f_initialize_0: xdr_float failed for index %d.\n",
		 col);
	    return 0;
	}

    for (row = 0; row < nofRows; row++)
	if (G__write_data(fd, row, nofCols) == -1) {
	    G_warning("G_random_f_initialize_0: write failed in row %d.\n",
		      row);
	    return 0;
	}

    return 1;
}

/*--------------------------------------------------------------------------*/
