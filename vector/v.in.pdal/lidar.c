
/****************************************************************************
 *
 * MODULE:       v.in.lidar
 * AUTHOR(S):    Vaclav Petras
 * PURPOSE:      common lidar-related definitions
 * COPYRIGHT:    (C) 2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the COPYING file that comes with GRASS
 *               for details.
 *
 *****************************************************************************/


#include "lidar.h"

void GLidarLayers_set_no_layers(struct GLidarLayers *layers)
{
    layers->id_layer = 0;
    layers->return_layer = 0;
    layers->class_layer = 0;
    layers->rgb_layer = 0;
}

struct class_table class_val[] = {
    {0, "Created, never classified"},
    {1, "Unclassified"},
    {2, "Ground"},
    {3, "Low Vegetation"},
    {4, "Medium Vegetation"},
    {5, "High Vegetation"},
    {6, "Building"},
    {7, "Low Point (noise)"},
    {8, "Model Key-point (mass point)"},
    {9, "Water"},
    {10, "Reserved for ASPRS Definition"},
    {11, "Reserved for ASPRS Definition"},
    {12, "Overlap Points"},
    {13 /* 13 - 31 */ , "Reserved for ASPRS Definition"},
    {0, 0}
};

struct class_table class_type[] = {
    {5, "Synthetic"},
    {6, "Key-point"},
    {7, "Withheld"},
    {0, 0}
};

int return_to_cat(int return_n, int n_returns)
{
    if (return_n == 1)
        return LAS_FIRST;
    else if (n_returns > 1 && return_n == n_returns)
            return LAS_LAST;
    else
        return LAS_MID;
}
