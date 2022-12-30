<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
/****************************************************************************
 *
 * MODULE:       r.in.pdal
 *               adapted from v.in.lidar
 * AUTHOR(S):    Vaclav Petras
 * PURPOSE:      common lidar-related definitions
 * COPYRIGHT:    (C) 2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the COPYING file that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
#ifndef GRASS_LIDAR_H
#define GRASS_LIDAR_H

#define LAS_ALL   0
#define LAS_FIRST 1
#define LAS_MID   2
#define LAS_LAST  3
<<<<<<< HEAD
=======

=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
#ifndef GRASS_LIDAR_H
#define GRASS_LIDAR_H

#define LAS_ALL   0
#define LAS_FIRST 1
<<<<<<< HEAD
#define LAS_MID 2
#define LAS_LAST 3
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
#define LAS_MID   2
#define LAS_LAST  3
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

/* Type and format specifier for count of points */
#ifdef HAVE_LONG_LONG_INT
typedef unsigned long long gpoint_count;

#define GPOINT_COUNT_FORMAT "%llu"
#else
typedef unsigned long gpoint_count;

#define GPOINT_COUNT_FORMAT "%lu"
#endif

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
struct GLidarLayers {
=======
struct GLidarLayers
{
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
struct GLidarLayers {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
struct GLidarLayers {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    int id_layer;
    int return_layer;
    int class_layer;
    int rgb_layer;
};

void GLidarLayers_set_no_layers(struct GLidarLayers *layers);

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

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
struct class_table {
=======
struct class_table
{
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
struct class_table {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
struct class_table {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    int code;
    char *name;
};

extern struct class_table class_val[];
extern struct class_table class_type[];

int return_to_cat(int return_n, int n_returns);

#endif /* GRASS_LIDAR_H */
