#ifndef __POINT_H__
#define __POINT_H__

/***************************************************************/
/*                                                             */
/*      point.h    in   r.spreadpath                           */
/*                                                             */
/*      This header file defines the point data structure      */
/*      and variables related to this structure.               */
/*                                                             */

/***************************************************************/

struct point
{
    int row, col, backrow, backcol;
    struct point *next;
};

#define POINT           struct point
#define PRES_PT_BACKROW PRES_PT->backrow
#define PRES_PT_BACKCOL PRES_PT->backcol
#define PRES_PT_ROW     PRES_PT->row
#define PRES_PT_COL     PRES_PT->col
#define NEXT_PT         PRES_PT->next
#define NEW_BACKROW     NEW_START_PT->backrow
#define NEW_BACKCOL     NEW_START_PT->backcol
#define NEW_ROW         NEW_START_PT->row
#define NEW_COL         NEW_START_PT->col
#define NEW_NEXT        NEW_START_PT->next
#define NEXT_START_PT   PRESENT_PT->next

#endif
