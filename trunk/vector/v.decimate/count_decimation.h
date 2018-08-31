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


#ifndef GRASS_COUNT_DECIMATION_H
#define GRASS_COUNT_DECIMATION_H

/* TODO: change int to ul/ull */
/* TODO: revise names (now partially on some vars in v.in.lidar code) */

struct CountDecimationControl {
    int offset_n;
    int offset_n_counter;
    int skip_every;
    int preserve_every;
    int every_counter;
    int n_count_filtered;
    int limit_n;
    int limit_n_counter;
};

void count_decimation_init(struct CountDecimationControl *control,
                           int *skip, int *preserve,
                           int *offset, int *limit);
int count_decimation_is_valid(struct CountDecimationControl *control);
int count_decimation_is_noop(struct CountDecimationControl *control);
void count_decimation_init_from_str(struct CountDecimationControl *control,
                                    const char *skip, const char *preserve,
                                    const char *offset, const char *limit);
int count_decimation_is_out(struct CountDecimationControl *control);
int count_decimation_is_end(struct CountDecimationControl *control);

#endif /* GRASS_COUNT_DECIMATION_H */
