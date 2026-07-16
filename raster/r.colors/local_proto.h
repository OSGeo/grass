/****************************************************************************
 *
 * MODULE:       r.colors
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *               David Johnson
 *
 * PURPOSE:      Allows creation and/or modification of the color table
 *               for a raster map layer.
 *
 * SPDX-FileCopyrightText: 2006 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 ***************************************************************************/

#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/raster3d.h>

struct maps_info {
    int num;
    char **names;
    char **mapsets;
    int *map_types;
    DCELL *min;
    DCELL *max;
};

#define RASTER_TYPE   1
#define RASTER3D_TYPE 2

/* stats.c */
int get_stats(struct maps_info *, struct Cell_stats *);
void get_fp_stats(struct maps_info *, struct FP_stats *statf, DCELL min,
                  DCELL max, int geometric, int geom_abs, int);

/* edit_colors.c */
int edit_colors(int, char **, int, const char *, const char *);

/* rules.c */
int read_color_rules(FILE *, struct Colors *, DCELL, DCELL, int, int *);
int check_percent_rule(const char *);
void rescale_colors(struct Colors *, struct Colors *, double, double);

#endif /* __LOCAL_PROTO_H__ */

