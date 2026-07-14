/****************************************************************************
 *
 * MODULE:       v.in.lidar
 * AUTHOR(S):    Vaclav Petras
 * PURPOSE:      vector mask
 * SPDX-FileCopyrightText: 2015 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
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

void VectorMask_init(struct VectorMask *vector_mask, const char *name,
                     const char *layer, int invert_mask);
void VectorMask_destroy(struct VectorMask *vector_mask);
int VectorMask_point_in(struct VectorMask *vector_mask, double x, double y);

#endif /* VECTOR_MASK_H */
