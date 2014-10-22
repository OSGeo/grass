/*
 **  Original Algorithm:    H. Mitasova, L. Mitas, J. Hofierka, M. Zlocha 
 **  GRASS Implementation:  J. Caplan, M. Ruesink  1995
 **
 **  US Army Construction Engineering Research Lab, University of Illinois 
 **
 **  Copyright  M. Ruesink, J. Caplan, H. Mitasova, L. Mitas, J. Hofierka, 
 **     M. Zlocha  1995
 **
 **This program is free software; you can redistribute it and/or
 **modify it under the terms of the GNU General Public License
 **as published by the Free Software Foundation; either version 2
 **of the License, or (at your option) any later version.
 **
 **This program is distributed in the hope that it will be useful,
 **but WITHOUT ANY WARRANTY; without even the implied warranty of
 **MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **GNU General Public License for more details.
 **
 **You should have received a copy of the GNU General Public License
 **along with this program; if not, write to the Free Software
 **Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 **
 */


#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "r.flow.h"
#include "io.h"
#include "mem.h"

/************************** MEMORY MGMT/ACCESS **************************/

void put_row_seg(layer l, int row)
{
    if (Segment_put_row(l.seg, l.buf[row] - l.col_offset,
			row + l.row_offset) < 1)
	G_fatal_error(_("Unable to write segment file for %s"), l.name);
}


void allocate_heap(void)
{
    int row;

    G_debug(1, "Allocating memory: elevation");

    /* 3 elevation buffers needed for precomputing aspects */

    el.buf =
	(DCELL **) G_calloc(region.rows + el.row_offset * 2 + 3,
			    sizeof(DCELL *));
    for (row = 0; row < 3; row++)
	el.buf[row] = ((DCELL *) G_calloc(region.cols + el.col_offset * 2,
					  sizeof(DCELL))) + el.row_offset;
    for (row = 3; row <= region.rows + el.row_offset; row++)
	el.buf[row] = parm.seg ? el.buf[row % 3] :
	    ((DCELL *) G_calloc(region.cols + el.col_offset * 2,
				sizeof(DCELL))) + el.row_offset;
    el.buf += el.col_offset;

    if (parm.seg) {
	G_debug(1, "Allocating memory: segment");
	el.seg = (SEGMENT *) G_malloc(sizeof(SEGMENT));
	Segment_init(el.seg, el.sfd, SEGSINMEM);
	as.seg = (SEGMENT *) G_malloc(sizeof(SEGMENT));
	Segment_init(as.seg, as.sfd, SEGSINMEM);
	if (parm.dsout) {
	    ds.seg = (SEGMENT *) G_malloc(sizeof(SEGMENT));
	    Segment_init(ds.seg, ds.sfd, SEGSINMEM);
	}
    }

    if (!parm.mem) {
	G_debug(1, "Allocating memory: aspect");
	as.buf = (DCELL **) G_calloc(region.rows, sizeof(DCELL *));
	as.buf[0] = (DCELL *) Rast_allocate_buf(DCELL_TYPE);
	for (row = 0; row < region.rows; row++)
	    as.buf[row] = parm.seg ?
		as.buf[0] : (DCELL *) Rast_allocate_buf(DCELL_TYPE);
    }

    if (parm.barin) {
	G_debug(1, "Allocating memory: barrier");
	bitbar = BM_create(region.cols, region.rows);
    }

    if (parm.dsout) {
	G_debug(1, "Allocating memory: density");
	ds.buf = (DCELL **) G_calloc(region.rows, sizeof(DCELL *));
	ds.buf[0] = (DCELL *) Rast_allocate_buf(DCELL_TYPE);
	for (row = 0; row < region.rows; row++)
	    ds.buf[row] = parm.seg ?
		ds.buf[0] : (DCELL *) Rast_allocate_buf(DCELL_TYPE);
    }

    if (parm.flout) {
	G_debug(1, "Allocating memory: flowline header");
	Vect_hist_command(&fl);
    }

    G_debug(1, "Allocating memory: e/w distances");
    ew_dist = (double *)G_calloc(region.rows, sizeof(double));

    G_debug(1, "Allocating memory: quantization tolerances");
    epsilon[HORIZ] = (double *)G_calloc(region.rows, sizeof(double));
    epsilon[VERT] = (double *)G_calloc(region.rows, sizeof(double));

    return;
}

void deallocate_heap(void)
{
    int row;

    G_debug(1, "De-allocating memory");

    if (parm.barin)
	BM_destroy(bitbar);
    G_free(el.buf[-1] - 1);

    if (parm.seg) {
	Segment_release(el.seg);
	if (!parm.mem)
	    Segment_release(as.seg);
	if (parm.dsout)
	    Segment_release(ds.seg);
    }
    else {
	G_free(el.buf[region.rows] - 1);
	for (row = 0; row < region.rows; row++)
	    G_free(el.buf[row] - 1);
    }
    G_free(--el.buf);

    if (!parm.mem) {
	for (row = 0; row < (parm.seg ? 1 : region.rows); row++)
	    G_free(as.buf[row]);
	G_free(as.buf);
    }
    G_free(ew_dist);
}
