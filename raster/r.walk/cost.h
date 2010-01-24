
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
    long age;
    int row;
    int col;
};

/* heap.c */
struct cost *insert(double, int, int);
struct cost *get_lowest(void);
int delete(struct cost *);
int init_heap(void);
int free_heap(void);

#endif /* __COST_H__ */
