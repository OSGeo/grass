
/***************************************************************/
/*                                                             */
/*      stash.h         in r.spreadpath                        */
/*                                                             */
/*       This header file declares the global variables and    */
/*       the structures that are to be used for command        */
/*       line processing.                                      */
/*                                                             */

/***************************************************************/

#include <stdio.h>
#define      BACKCOL_LAYER      1
#define      BACKROW_LAYER      2
#define      START_PT           3
#define      PATH_LAYER         4

#include "point.h"
#ifdef MAIN

struct variables
{
    char *alias;
    int position;
}

variables[] = {
    {
    "x_input", BACKCOL_LAYER}, {
    "y_input", BACKROW_LAYER}, {
    "coor", START_PT}, {
    "output", PATH_LAYER}
};

char path_layer[64];
char backrow_layer[64];
char backcol_layer[64];
struct point *head_start_pt = NULL;

#else

extern char path_layer[];
extern char backrow_layer[];
extern char backcol_layer[];
extern struct point *head_start_pt;

#endif

/****************END OF "GDRAIN_CMD_LINE.H"**********************/
