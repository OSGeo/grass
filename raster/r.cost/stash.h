
/****************************************************************************
 *
 * MODULE:       r.cost
 *
 * AUTHOR(S):    Antony Awaida - IESL - M.I.T.
 *               James Westervelt - CERL
 *               Pierre de Mouveaux <pmx audiovu com>
 *               Eric G. Miller <egm2 jps net>
 *
 * PURPOSE:      Outputs a raster map layer showing the cumulative cost
 *               of moving between different geographic locations on an
 *               input raster map layer whose cell category values
 *               represent cost.
 *
 * COPYRIGHT:    (C) 2006-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#ifndef __STASH_H__
#define __STASH_H__

#include <stdio.h>

#define      CUM_COST_LAYER        1
#define      COST_LAYER            2
#define      START_PT              3
#define      MOVE_DIR_LAYER        4

struct start_pt
{
    int row;
    int col;
    int value;
    struct start_pt *next;
};

extern char cum_cost_layer[], move_dir_layer[];
extern char cost_layer[];
extern struct start_pt *head_start_pt;
extern struct start_pt *head_end_pt;

int process_answers(char **, struct start_pt **, struct start_pt **);
int time_to_stop(int, int);

#endif /* __STASH_H__ */
