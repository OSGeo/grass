
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

extern char path_layer[];
extern char backrow_layer[];
extern char backcol_layer[];
extern struct point *head_start_pt;

/****************END OF "GDRAIN_CMD_LINE.H"**********************/
