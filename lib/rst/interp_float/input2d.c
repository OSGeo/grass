
/*!
 * \file input2d.c
 *
 * \author H. Mitasova, I. Kosinovsky, D. Gerdes Fall 1993 (original authors)
 * \author modified by McCauley in August 1995
 * \author modified by Mitasova in August 1995
 * \author modified by Brown in June 1999 - added elatt & smatt
 *
 * \copyright
 * (C) 1993-1999 by Helena Mitasova and the GRASS Development Team
 *
 * \copyright
 * This program is free software under the
 * GNU General Public License (>=v2).
 * Read the file COPYING that comes with GRASS
 * for details.
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/bitmap.h>
#include <grass/linkm.h>
#include <grass/interpf.h>
#include <grass/glocale.h>


/*!
 * Creates a bitmap mask from given raster map
 *
 * Creates a bitmap mask from maskmap raster file and/or current MASK if
 * present and returns a pointer to the bitmask. If no mask is in force
 * returns NULL.
 */
struct BM *IL_create_bitmask(struct interp_params *params)
{
    int i, j, cfmask = -1, irev, MASKfd;
    const char *mapsetm;
    CELL *cellmask, *MASK;
    struct BM *bitmask;

    if ((MASKfd = Rast_maskfd()) >= 0)
	MASK = Rast_allocate_c_buf();
    else
	MASK = NULL;

    if (params->maskmap != NULL || MASK != NULL) {
	bitmask = BM_create(params->nsizc, params->nsizr);

	if (params->maskmap != NULL) {
	    mapsetm = G_find_raster2(params->maskmap, "");
	    if (!mapsetm)
		G_fatal_error(_("Mask raster map <%s> not found"),
			      params->maskmap);

	    cellmask = Rast_allocate_c_buf();
	    cfmask = Rast_open_old(params->maskmap, mapsetm);
	}
	else
	    cellmask = NULL;

	for (i = 0; i < params->nsizr; i++) {
	    irev = params->nsizr - i - 1;
	    if (cellmask)
		Rast_get_c_row(cfmask, cellmask, i);
	    if (MASK)
		Rast_get_c_row(MASKfd, MASK, i);
	    for (j = 0; j < params->nsizc; j++) {
		if ((cellmask && (cellmask[j] == 0 || Rast_is_c_null_value(&cellmask[j]))) || 
		    (MASK && (MASK[j] == 0 || Rast_is_c_null_value(&MASK[j]))))
		    BM_set(bitmask, j, irev, 0);
		else
		    BM_set(bitmask, j, irev, 1);
	    }
	}
	G_message(_("Bitmap mask created"));
    }
    else
	bitmask = NULL;

    if (cfmask >= 0)
	Rast_close(cfmask);

    return bitmask;
}

int translate_quad(struct multtree *tree,
		   double numberx,
		   double numbery, double numberz, int n_leafs)
{
    int total = 0, i, ii;

    if (tree == NULL)
	return 0;
    if (tree->data == NULL)
	return 0;

    if (tree->leafs != NULL) {
	((struct quaddata *)(tree->data))->x_orig -= numberx;
	((struct quaddata *)(tree->data))->y_orig -= numbery;
	((struct quaddata *)(tree->data))->xmax -= numberx;
	((struct quaddata *)(tree->data))->ymax -= numbery;
	for (ii = 0; ii < n_leafs; ii++)
	    total +=
		translate_quad(tree->leafs[ii], numberx, numbery, numberz,
			       n_leafs);
    }
    else {
	((struct quaddata *)(tree->data))->x_orig -= numberx;
	((struct quaddata *)(tree->data))->y_orig -= numbery;
	((struct quaddata *)(tree->data))->xmax -= numberx;
	((struct quaddata *)(tree->data))->ymax -= numbery;
	for (i = 0; i < ((struct quaddata *)(tree->data))->n_points; i++) {
	    ((struct quaddata *)(tree->data))->points[i].x -= numberx;
	    ((struct quaddata *)(tree->data))->points[i].y -= numbery;
	    ((struct quaddata *)(tree->data))->points[i].z -= numberz;
	}

	return 1;
    }

    return total;
}
