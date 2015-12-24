/*
 * v.in.lidar filtering functions
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


#include "filters.h"

#include "lidar.h"

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

void return_filter_create_from_string(struct ReturnFilter *return_filter,
                                      const char *name)
{
    return_filter->filter = LAS_ALL;
    if (name) {
        if (strcmp(name, "first") == 0)
            return_filter->filter = LAS_FIRST;
        else if (strcmp(name, "last") == 0)
            return_filter->filter = LAS_LAST;
        else if (strcmp(name, "mid") == 0)
            return_filter->filter = LAS_MID;
        else
            G_fatal_error(_("Unknown return filter value <%s>"), name);
    }
}

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
