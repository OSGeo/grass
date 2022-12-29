/*
 * lidar-related filtering functions
 *
 * Authors:
 *  Markus Metz (r.in.lidar)
 *  Vaclav Petras (refactoring and various additions)
 *
 * Copyright 2011-2016 by Markus Metz, and The GRASS Development Team
 *
 * This program is free software licensed under the GPL (>=v2).
 * Read the COPYING file that comes with GRASS for details.
 *
 */

#ifndef __FILTERS_H__
#define __FILTERS_H__

#define LAS_ALL 0
#define LAS_FIRST 1
#define LAS_MID 2
#define LAS_LAST 3

struct ReturnFilter
{
    int filter;
};

struct ClassFilter
{

    /** NULL terminated list of class numbers represented as string */
    char **str_classes;
};

struct Option;

int range_filter_from_option(struct Option *option, double *min, double *max);

int return_filter_create_from_string(struct ReturnFilter *return_filter,
                                     const char *name);
int return_filter_is_out(struct ReturnFilter *return_filter, int return_n,
                         int n_returns);
int class_filter_create_from_strings(struct ClassFilter *class_filter,
                                     char **classes);
int class_filter_is_out(struct ClassFilter *class_filter, int class_n);

#endif /* __FILTERS_H__ */
