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

#include "local_proto.h"
#include "filters.h"

#include <stdlib.h>
#include <grass/gis.h>

int return_filter_is_out(struct ReturnFilter *return_filter, int return_n,
                         int n_returns)
{
    if (return_filter->filter == LAS_ALL)
        return FALSE;
    int skipme = 1;

    switch (return_filter->filter) {
    case LAS_FIRST:
        if (return_n == 1)
            skipme = 0;
        break;
    case LAS_MID:
        if (return_n > 1 && return_n < n_returns)
            skipme = 0;
        break;
    case LAS_LAST:
        if (n_returns > 1 && return_n == n_returns)
            skipme = 0;
        break;
    }
    if (skipme)
        return TRUE;
    return FALSE;
}

void class_filter_create_from_strings(struct ClassFilter *class_filter,
                                      char **classes)
{
    class_filter->str_classes = classes;
}

int class_filter_is_out(struct ClassFilter *class_filter, int class_n)
{
    if (!class_filter->str_classes)
        return FALSE;
    int i = 0;
    int skipme = TRUE;

    while (class_filter->str_classes[i]) {
        if (class_n == atoi(class_filter->str_classes[i])) {
            skipme = FALSE;
            break;
        }
        i++;
    }
    if (skipme)
        return TRUE;
    return FALSE;
}
