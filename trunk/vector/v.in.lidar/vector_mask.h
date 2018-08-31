
/****************************************************************************
 *
 * MODULE:       v.in.lidar
 * AUTHOR(S):    Vaclav Petras
 * PURPOSE:      vector mask
 * COPYRIGHT:    (C) 2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the COPYING file that comes with GRASS
 *               for details.
 *
 *****************************************************************************/


#ifndef VECTOR_MASK_H
#define VECTOR_MASK_H

struct Map_info;
struct bound_box;

struct VectorMask {
    struct Map_info *map_info;
    struct bound_box *map_bbox;
    struct bound_box *area_bboxes;
    int nareas;
    int inverted;
};

void VectorMask_init(struct VectorMask *vector_mask, const char *name, const char *layer, int invert_mask);
void VectorMask_destroy(struct VectorMask *vector_mask);
int VectorMask_point_in(struct VectorMask *vector_mask, double x, double y);

#endif /* VECTOR_MASK_H */
