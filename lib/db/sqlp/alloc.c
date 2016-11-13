
/*****************************************************************************
*
* MODULE:       SQL statement parser library 
*   	    	
* AUTHOR(S):    lex.l and yac.y were originally taken from unixODBC and
*               probably written by Peter Harvey <pharvey@codebydesigns.com>,
*               modifications and other code by Radim Blazek
*
* PURPOSE:      Parse input string containing SQL statement to 
*               SQLPSTMT structure.
*               SQL parser may be used by simple database drivers. 
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <grass/sqlp.h>

/* alloc structure */
SQLPSTMT *sqpInitStmt(void)
{
    SQLPSTMT *st;

    st = (SQLPSTMT *) calloc(1, sizeof(SQLPSTMT));

    return (st);
}

/* allocate space for columns */
int sqpAllocCol(SQLPSTMT * st, int n)
{
    int i;

    if (n > st->aCol) {
	n += 15;
	st->Col = (SQLPVALUE *) realloc(st->Col, n * sizeof(SQLPVALUE));
	st->ColType = (int *)realloc(st->ColType, n * sizeof(int));
	st->ColWidth = (int *)realloc(st->ColWidth, n * sizeof(int));
	st->ColDecim = (int *)realloc(st->ColDecim, n * sizeof(int));

	for (i = st->nCol; i < n; i++) {
	    st->Col[i].s = NULL;
	}

	st->aCol = n;
    }
    return (1);
}

/* allocate space for values */
int sqpAllocVal(SQLPSTMT * st, int n)
{
    int i;

    if (n > st->aVal) {
	n += 15;
	st->Val = (SQLPVALUE *) realloc(st->Val, n * sizeof(SQLPVALUE));

	for (i = st->nVal; i < n; i++) {
	    st->Val[i].s = NULL;
	}

	st->aVal = n;
    }
    return (1);
}

/* free space allocated by parser */
int sqpFreeStmt(SQLPSTMT * st)
{
    int i;

    /* columns */
    for (i = 0; i < st->aCol; i++)
	free(st->Col[i].s);

    free(st->Col);
    free(st->ColType);
    free(st->ColWidth);
    free(st->ColDecim);
    st->aCol = 0;
    st->nCol = 0;

    /* values */
    for (i = 0; i < st->aVal; i++)
	free(st->Val[i].s);

    free(st->Val);
    st->aVal = 0;
    st->nVal = 0;

    free(st->orderCol);

    /* Nodes (where) */
    if (st->upperNodeptr)
	sqpFreeNode(st->upperNodeptr);

    free(st);
    return (1);
}
