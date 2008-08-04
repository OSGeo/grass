
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
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

/***************************************************************/
/*                                                             */
/*        cost.h    in   ~/src/Gcost                           */
/*                                                             */
/*      This header file defines the data structure of a       */
/*      point structure containing various attributes of       */
/*      a grid cell.                                           */
/*                                                             */

/***************************************************************/

#ifndef __COST_H__
#define __COST_H__

struct cost
{
    double min_cost;
    int row;
    int col;
    struct cost *lower;
    struct cost *higher;
    struct cost *above;
    struct cost *nexttie;
    struct cost *previoustie;
};

/* btree.c */
struct cost *insert(double, int, int);
struct cost *find(double, int, int);
struct cost *get_lowest(void);
int show(struct cost *);
int show_all();
int delete(struct cost *);
int check(char *, struct cost *);

#endif /* __COST_H__ */
