
/***************************************************************/
/*                                                             */
/*       stash.h     in    ~/src/Gcost                         */
/*                                                             */
/*       This header file declares the global variables and    */
/*       the structures that are to be used for command        */
/*       line processing.                                      */
/*                                                             */

/***************************************************************/

#ifndef __R_COST_STASH_H__
#define __R_COST_STASH_H__

#include <stdio.h>
#define      CUM_COST_LAYER        1
#define      COST_LAYER            2
#define      START_PT              3

struct start_pt
{
    int row;
    int col;
    struct start_pt *next;
};

extern struct start_pt *head_start_pt;
extern struct start_pt *head_end_pt;

int process_answers(char **, struct start_pt **, struct start_pt **);
int time_to_stop(int, int);

#endif

/****************END OF "GCOST_CMD_LINE.H"**********************/
