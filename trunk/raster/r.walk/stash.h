
/***************************************************************/
/*                                                             */
/*       stash.h     in    ~/src/Gcost                         */
/*                                                             */
/*       This header file declares the global variables and    */
/*       the structures that are to be used for command        */
/*       line processing.                                      */
/*                                                             */

/***************************************************************/

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

struct start_pt *process_start_coords(char **, struct start_pt *);
int process_stop_coords(char **answers);
int time_to_stop(int, int);

#endif /* __STASH_H__ */
