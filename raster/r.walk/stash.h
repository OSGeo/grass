
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

#ifdef MAIN

struct variables
{
    char *alias;
    int position;
}

variables[] = {

    {
    "output", CUM_COST_LAYER}, {
    "input", COST_LAYER}, {
    "coor", START_PT}
};

char cum_cost_layer[64];
char cost_layer[64], dtm_layer[64];
struct start_pt *head_start_pt = NULL;
struct start_pt *head_end_pt = NULL;

#else

extern char cum_cost_layer[];
extern char cost_layer[], dtm_layer[];
extern struct start_pt *head_start_pt;
extern struct start_pt *head_end_pt;

#endif

int process_answers(char **, struct start_pt **, struct start_pt **);
int time_to_stop(int, int);

#endif

/****************END OF "GCOST_CMD_LINE.H"**********************/
