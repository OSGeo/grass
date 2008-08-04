
/***************************************************************/
/*                                                             */
/*        cost.h    in   ~/src/Gcost                           */
/*                                                             */
/*      This header file defines the data structure of a       */
/*      point structure containing various attributes of       */
/*      a grid cell.                                           */
/*                                                             */

/***************************************************************/

#ifndef __R_COST_H__
#define __R_COST_H__

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

#endif

/***************************************************************/
