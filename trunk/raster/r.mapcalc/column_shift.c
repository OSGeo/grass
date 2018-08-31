#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "expression.h"
#include "globals.h"


void column_shift(void *buf, int res_type, int col)
{
    CELL *ibuf = buf;
    FCELL *fbuf = buf;
    DCELL *dbuf = buf;
    int i;

    /* if column offset, copy cell to itself shifting by col */
    if (col > 0) {
	switch (res_type) {
	case CELL_TYPE:
	    for (i = 0; i < columns - col; i++) {
		if (IS_NULL_C(&ibuf[i + col]))
		    SET_NULL_C(&ibuf[i]);
		else
		    ibuf[i] = ibuf[i + col];
	    }

	    for (; i < columns; i++)
		SET_NULL_C(&ibuf[i]);
	    break;

	case FCELL_TYPE:
	    for (i = 0; i < columns - col; i++) {
		if (IS_NULL_F(&fbuf[i + col]))
		    SET_NULL_F(&fbuf[i]);
		else
		    fbuf[i] = fbuf[i + col];
	    }

	    for (; i < columns; i++)
		SET_NULL_F(&fbuf[i]);
	    break;

	case DCELL_TYPE:
	    for (i = 0; i < columns - col; i++) {
		if (IS_NULL_D(&dbuf[i + col]))
		    SET_NULL_D(&dbuf[i]);
		else
		    dbuf[i] = dbuf[i + col];
	    }

	    for (; i < columns; i++)
		SET_NULL_D(&dbuf[i]);
	    break;
	}
    }
    else if (col < 0) {
	col = -col;

	switch (res_type) {
	case CELL_TYPE:
	    for (i = columns - 1; i >= col; i--) {
		if (IS_NULL_C(&ibuf[i - col]))
		    SET_NULL_C(&ibuf[i]);
		else
		    ibuf[i] = ibuf[i - col];
	    }

	    for (; i >= 0; i--)
		SET_NULL_C(&ibuf[i]);
	    break;

	case FCELL_TYPE:
	    for (i = columns - 1; i >= col; i--) {
		if (IS_NULL_F(&fbuf[i - col]))
		    SET_NULL_F(&fbuf[i]);
		else
		    fbuf[i] = fbuf[i - col];
	    }

	    for (; i >= 0; i--)
		SET_NULL_F(&fbuf[i]);
	    break;

	case DCELL_TYPE:
	    for (i = columns - 1; i >= col; i--) {
		if (IS_NULL_D(&dbuf[i - col]))
		    SET_NULL_D(&dbuf[i]);
		else
		    dbuf[i] = dbuf[i - col];
	    }

	    for (; i >= 0; i--)
		SET_NULL_D(&dbuf[i]);
	    break;
	}
    }
}
