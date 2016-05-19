
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


#ifndef GRASS_LIDAR_H
#define GRASS_LIDAR_H

#define LAS_ALL 0
#define LAS_FIRST 1
#define LAS_MID 2
#define LAS_LAST 3

/*
 * ASPRS Standard LIDAR Point Classes
 * Classification Value (bits 0:4) : Meaning
 *      0 : Created, never classified
 *      1 : Unclassified
 *      2 : Ground
 *      3 : Low Vegetation
 *      4 : Medium Vegetation
 *      5 : High Vegetation
 *      6 : Building
 *      7 : Low Point (noise)
 *      8 : Model Key-point (mass point)
 *      9 : Water
 *     10 : Reserved for ASPRS Definition
 *     11 : Reserved for ASPRS Definition
 *     12 : Overlap Points
 *  13-31 : Reserved for ASPRS Definition
 */

/* Classification Bit Field Encoding
 * Bits | Field Name     | Description
 *  0-4 | Classification | Standard ASPRS classification as defined in the
 *                         above classification table.
 *    5 | Synthetic      | If set then this point was created by a technique
 *                         other than LIDAR collection such as digitized from
 *                         a photogrammetric stereo model or by traversing
 *                         a waveform.
 *    6 | Key-point      | If set, this point is considered to be a model
 *                         key-point and thus generally should not be withheld
 *                         in a thinning algorithm.
 *    7 | Withheld       | If set, this point should not be included in
 *                         processing (synonymous with Deleted).
 */

/* keep the comments above in sync with the .c file */

struct class_table
{
    int code;
    char *name;
};

extern struct class_table class_val[];
extern struct class_table class_type[];

int return_to_cat(int return_n, int n_returns);

#endif /* GRASS_LIDAR_H */
