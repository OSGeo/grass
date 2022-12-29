/****************************************************************************
 *
 * MODULE:       v.decimate
 * AUTHOR(S):    Vaclav Petras
 * PURPOSE:      Reduce the number of points in a vector map
 * COPYRIGHT:    (C) 2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the COPYING file that comes with GRASS
 *               for details.
 *
 *****************************************************************************/


/* TODO: change int */
/* TODO: revise names */

#include "count_decimation.h"

#include <grass/gis.h>

#include <stdlib.h>


void count_decimation_init(struct CountDecimationControl *control,
                           int *skip, int *preserve,
                           int *offset, int *limit)
{
    control->skip_every = 0;
    control->preserve_every = 0;
    /* counter used by both but that's ok, skip and preserve are exclusive */
    control->every_counter = 0;
    control->n_count_filtered = 0;
    control->offset_n = 0;
    control->offset_n_counter = 0;
    control->limit_n = 0;
    control->limit_n_counter = 0;
    if (skip)
        control->skip_every = *skip;
    if (preserve)
        control->preserve_every = *preserve;
    if (offset)
        control->offset_n = *offset;
    if (limit)
        control->limit_n = *limit;
}


int count_decimation_is_valid(struct CountDecimationControl *control)
{
    if (control->skip_every == 1)
        return FALSE;
    if (control->skip_every && control->preserve_every > 1)
        return FALSE;
    return TRUE;
}


int count_decimation_is_noop(struct CountDecimationControl *control)
{
    if (control->skip_every < 2 && control->preserve_every < 2
            && !control->offset_n && !control->limit_n)
        return TRUE;
    return FALSE;
}

void count_decimation_init_from_str(struct CountDecimationControl *control,
                                    const char *skip, const char *preserve,
                                    const char *offset, const char *limit)
{
    control->skip_every = 0;
    control->preserve_every = 0;
    control->every_counter = 0;
    control->n_count_filtered = 0;
    control->offset_n = 0;
    control->offset_n_counter = 0;
    control->limit_n = 0;
    control->limit_n_counter = 0;
    /* TODO: atoi is probably not appropriate */
    if (skip)
        control->skip_every = atoi(skip);
    if (preserve)
        control->preserve_every = atoi(preserve);
    if (offset)
        control->offset_n = atoi(offset);
    if (limit)
        control->limit_n = atoi(limit);
}


/* TODO: eliminate noop cases */
int count_decimation_is_out(struct CountDecimationControl *control)
{
    if (control->offset_n) {
        if (control->offset_n_counter < control->offset_n) {
            control->offset_n_counter++;
            return TRUE;
        }
        else {
            control->offset_n = 0;  /* disable offset check */
        }
    }
    if (control->skip_every) {
        control->every_counter++;
        if (control->every_counter == control->skip_every) {
            control->n_count_filtered++;
            control->every_counter = 0;
            return TRUE;
        }
    }
    else if (control->preserve_every) {
        control->every_counter++;
        if (control->every_counter == control->preserve_every) {
            control->every_counter = 0;
        }
        else {
            control->n_count_filtered++;
            return TRUE;
        }
    }
    return FALSE;
}


int count_decimation_is_end(struct CountDecimationControl *control)
{
    if (control->limit_n) {
        control->limit_n_counter++;
        /* this matches the last successfully imported point */
        if (control->limit_n_counter == control->limit_n)
            return TRUE;
    }
    return FALSE;
}
