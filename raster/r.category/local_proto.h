/****************************************************************************
 *
 * MODULE:       r.cats
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Prints category values and labels associated with
 *               user-specified raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/parson.h>

enum OutputFormat { PLAIN, JSON };

/* cats.c */
int get_cats(const char *, const char *);
int next_cat(long *);

/* main.c */
void print_json(JSON_Value *);
int print_label(long, enum OutputFormat, JSON_Array *);
int print_d_label(double, enum OutputFormat, JSON_Array *);
int scan_cats(const char *, long *, long *);
int scan_vals(const char *, double *);

#endif /* __LOCAL_PROTO_H__ */
