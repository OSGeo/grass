
/***************************************************************************
* MODULE:       this structs/functions are used by r3.mask and r3.null
*
* AUTHOR(S):    Roman Waupotitsch, Michael Shapiro, Helena Mitasova,
*		Bill Brown, Lubos Mitas, Jaro Hofierka
*
* COPYRIGHT:    (C) 2005 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/
/*Headerfile for global structs and funcktions */

#ifndef __MASK_FUNCTIONS_H__
#define __MASK_FUNCTIONS_H__
/*Structures */
typedef struct _d_interval
{
    double low, high;
    int inf;
    struct _d_interval *next;
} d_Interval;

typedef struct _d_mask
{
    d_Interval *list;
} d_Mask;

/*Prototypes for mask_functions.c */
int mask_d_select(DCELL * x, d_Mask * mask);
extern DCELL mask_match_d_interval(DCELL x, d_Interval * I);
void parse_vallist(char **vallist, d_Mask ** d_mask);

#endif
