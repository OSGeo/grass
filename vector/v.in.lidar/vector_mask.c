/*
 * v.in.lidar vector mask
 *
 * Copyright 2011-2015 by Vaclav Petras, and the GRASS Development Team
 *
 * This program is free software licensed under the GPL (>=v2).
 * Read the COPYING file that comes with GRASS for details.
 *
 */

#include <grass/vector.h>
#include <grass/glocale.h>

#include "vector_mask.h"

void VectorMask_init(struct VectorMask *vector_mask, const char *name,
                     const char *layer, int invert_mask)
{
    vector_mask->map_info = G_malloc(sizeof(struct Map_info));
    if (Vect_open_old2(vector_mask->map_info, name, "", layer) < 2)
        G_fatal_error(_("Failed to open vector <%s>"), name);
    vector_mask->map_bbox = G_malloc(sizeof(struct bound_box));
    Vect_get_map_box(vector_mask->map_info, vector_mask->map_bbox);
    vector_mask->nareas = Vect_get_num_areas(vector_mask->map_info);
    vector_mask->area_bboxes =
        G_malloc(vector_mask->nareas * sizeof(struct bound_box));
    int i;

    for (i = 1; i <= vector_mask->nareas; i++) {
        Vect_get_area_box(vector_mask->map_info, i,
                          &vector_mask->area_bboxes[i - 1]);
    }
    if (invert_mask)
        vector_mask->inverted = 1;
    else
        vector_mask->inverted = 0;
}

void VectorMask_destroy(struct VectorMask *vector_mask)
{
    G_free(vector_mask->map_bbox);
    G_free(vector_mask->area_bboxes);
    Vect_close(vector_mask->map_info);
    G_free(vector_mask->map_info);
}

int VectorMask_point_in(struct VectorMask *vector_mask, double x, double y)
{
    /* inv in res
     *   F  T continue
     *   F  F return F
     *   T  T continue
     *   T  F return T
     */
    if (!Vect_point_in_box_2d(x, y, vector_mask->map_bbox))
        return vector_mask->inverted;
    int is_out = TRUE;
    int i;

    for (i = 1; i <= vector_mask->nareas; i++) {
        if (Vect_point_in_area(x, y, vector_mask->map_info, i,
                               &vector_mask->area_bboxes[i - 1])) {
            is_out = FALSE;
            break;
        }
    }
    /* inv out res
     *  F   T   F
     *  F   F   T
     *  T   T   T
     *  T   F   F
     */
    if (vector_mask->inverted ^ is_out)
        return FALSE;
    return TRUE;
}
