/*
 * r.in.lidar filtering functions
 *
 * Copyright 2011-2015 by Markus Metz, and The GRASS Development Team
 * Authors:
 *  Markus Metz (r.in.lidar)
 *  Vaclav Petras (move code to a separate files)
 *
 * This program is free software licensed under the GPL (>=v2).
 * Read the COPYING file that comes with GRASS for details.
 *
 */

#ifndef __FILTERS_H__
#define __FILTERS_H__

struct ReturnFilter
{
    int filter;
};

struct ClassFilter
{

    /** NULL terminated list of class numbers represented as string */
    char **str_classes;
};

int return_filter_is_out(struct ReturnFilter *return_filter, int return_n,
                         int n_returns);
void class_filter_create_from_strings(struct ClassFilter *class_filter,
                                      char **classes);
int class_filter_is_out(struct ClassFilter *class_filter, int class_n);

#endif /* __FILTERS_H__ */
