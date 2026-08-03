/*
 * r.in.lidar filtering functions
 *
 * SPDX-FileCopyrightText: 2011-2015 Markus Metz
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Authors:
 *  Markus Metz (r.in.lidar)
 *  Vaclav Petras (move code to a separate files)
 *
 */

#ifndef __FILTERS_H__
#define __FILTERS_H__

struct ReturnFilter {
    int filter;
};

struct ClassFilter {

    /** NULL terminated list of class numbers represented as string */
    char **str_classes;
};

int return_filter_is_out(struct ReturnFilter *return_filter, int return_n,
                         int n_returns);
void class_filter_create_from_strings(struct ClassFilter *class_filter,
                                      char **classes);
int class_filter_is_out(struct ClassFilter *class_filter, int class_n);

#endif /* __FILTERS_H__ */
