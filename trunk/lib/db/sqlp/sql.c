
/*****************************************************************************
*
* MODULE:       SQL statement parser library 
*   	    	
* AUTHOR(S):    lex.l and yac.y were originally taken from unixODBC and
*               probably written by Peter Harvey <pharvey@codebydesigns.com>,
*               original modifications & added code by 
*                     Radim Blazek <radim.blazek gmail.com>
*               Glynn Clements <glynn gclements.plus.com>,
*               Markus Neteler <neteler itc.it>,
*               Martin Landa <landa.martin gmail.com>,
*               Moritz Lennert <mlennert club.worldonline.be>,
*               Hamish Bowman <hamish_b yahoo.com>,
*               Daniel Calvelo Aros <dca.gis gmail.com>,
*               Paul Kelly <paul-grass stjohnspoint.co.uk>,
*               Alex Shevlakov <sixote yahoo.com>
*
* PURPOSE:      Parse input string containing SQL statement to 
*               SQLPSTMT structure.
*               SQL parser may be used by simple database drivers. 
*
* COPYRIGHT:    (C) 2000-2007 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <grass/sqlp.h>

SQLPSTMT *sqlpStmt;

/* save string to value */
int sqpSaveStr(SQLPVALUE * val, char *c)
{
    int len = 0;

    len = strlen(c) + 1;
    val->s = (char *)realloc(val->s, len);

    strcpy(val->s, c);

    return (1);
}

void sqpInitValue(SQLPVALUE * val)
{
    val->type = SQLP_NULL;
    val->s = NULL;
    val->i = 0;
    val->d = 0.0;
}

void sqpCopyValue(SQLPVALUE * from, SQLPVALUE * to)
{
    to->type = from->type;

    if (to->s)
	free(to->s);

    if (from->s)
	to->s = strdup(from->s);

    to->i = from->i;
    to->d = from->d;
}

int sqpInitParser(SQLPSTMT * st)
{
    sqlpStmt = st;
    sqlpStmt->cur = sqlpStmt->stmt;

    sqlpStmt->errmsg[0] = '\0';
    sqlpStmt->table[0] = '\0';
    sqlpStmt->nCol = 0;
    sqlpStmt->nVal = 0;
    sqlpStmt->upperNodeptr = NULL;
    sqlpStmt->orderCol = NULL;

    return (1);
}

void sqpCommand(int command)
{
    sqlpStmt->command = command;
    return;
}

void sqpTable(char *tbl)
{
    strncpy(sqlpStmt->table, tbl, SQLP_MAX_TABLE);
    return;
}

void sqpColumn(char *col)
{
    int i;

    i = sqlpStmt->nCol;
    sqpAllocCol(sqlpStmt, i + 1);
    sqpSaveStr(&(sqlpStmt->Col[i]), col);

    sqlpStmt->nCol++;
    return;
}

void sqpColumnDef(char *col, int type, int width, int decimals)
{
    int i;

    i = sqlpStmt->nCol;
    sqpAllocCol(sqlpStmt, i + 1);
    sqpSaveStr(&(sqlpStmt->Col[i]), col);
    sqlpStmt->ColType[i] = type;
    sqlpStmt->ColWidth[i] = width;
    sqlpStmt->ColDecim[i] = decimals;

    sqlpStmt->nCol++;
    return;
}

void sqpValue(char *strval, int intval, double dblval, int type)
{
    int i;

    i = sqlpStmt->nVal;

    /* allocate space for cols because if in INSERT cols were not
     * specified array for ColNum would not be allocated */
    sqpAllocCol(sqlpStmt, i + 1);

    sqpAllocVal(sqlpStmt, i + 1);
    sqlpStmt->Val[i].s = NULL;
    sqlpStmt->Val[i].i = 0;	/* not necessay I think */
    sqlpStmt->Val[i].d = 0.0;	/* not necessay I think */

    sqlpStmt->Val[i].type = type;
    switch (type) {
    case (SQLP_S):
	sqpSaveStr(&(sqlpStmt->Val[i]), strval);
	break;
    case (SQLP_I):
	sqlpStmt->Val[i].i = intval;
	break;
    case (SQLP_D):
	sqlpStmt->Val[i].d = dblval;
	break;
	/* SQLP_NULL, nothing to do */
    }

    sqlpStmt->nVal++;
    return;
}

void sqpAssignment(char *col, char *strval, int intval, double dblval,
		   SQLPNODE * expval, int type)
{
    int i;

    i = sqlpStmt->nCol;

    sqpAllocCol(sqlpStmt, i + 1);
    sqpSaveStr(&(sqlpStmt->Col[i]), col);

    sqpAllocVal(sqlpStmt, i + 1);
    sqlpStmt->Val[i].s = NULL;
    sqlpStmt->Val[i].i = 0;	/* not necessay I think */
    sqlpStmt->Val[i].d = 0.0;	/* not necessay I think */

    sqlpStmt->Val[i].type = type;
    switch (type) {
    case (SQLP_S):
	sqpSaveStr(&(sqlpStmt->Val[i]), strval);
	break;
    case (SQLP_I):
	sqlpStmt->Val[i].i = intval;
	break;
    case (SQLP_D):
	sqlpStmt->Val[i].d = dblval;
	break;
    case (SQLP_EXPR):
	sqlpStmt->Val[i].expr = expval;
	/* Don't do anything right now; come back to this when executing */
	break;
	/* SQLP_NULL, nothing to do */
    }

    sqlpStmt->nCol++;
    sqlpStmt->nVal++;
    return;
}

void sqpOrderColumn(char *col, int dir)
{
    sqlpStmt->orderCol = (char *)realloc(sqlpStmt->orderCol, strlen(col) + 1);
    strcpy(sqlpStmt->orderCol, col);
    sqlpStmt->orderDir = dir;
    return;
}

/* Create and init new node */
SQLPNODE *sqpNewNode(void)
{
    SQLPNODE *np;

    np = (SQLPNODE *) calloc(1, sizeof(SQLPNODE));
    return np;
}

SQLPNODE *sqpNewExpressionNode(int oper, SQLPNODE * left, SQLPNODE * right)
{
    SQLPNODE *np;

    np = sqpNewNode();

    np->node_type = SQLP_NODE_EXPRESSION;
    np->oper = oper;
    np->left = left;
    np->right = right;

    return np;
}

SQLPNODE *sqpNewColumnNode(char *name)
{
    SQLPNODE *np;

    np = sqpNewNode();

    np->node_type = SQLP_NODE_COLUMN;
    np->column_name = strdup(name);

    return np;
}

SQLPNODE *sqpNewValueNode(char *strval, int intval, double dblval, int type)
{
    SQLPNODE *np;

    np = sqpNewNode();

    np->node_type = SQLP_NODE_VALUE;

    np->value.type = type;
    if (strval)
	np->value.s = strdup(strval);
    np->value.i = intval;
    np->value.d = dblval;

    return np;
}

void sqpFreeNode(SQLPNODE * np)
{
    if (!np)
	return;

    if (np->left)
	sqpFreeNode(np->left);

    if (np->right)
	sqpFreeNode(np->right);

    if (np->column_name)
	free(np->column_name);

    if (np->value.s)
	free(np->value.s);

    free(np);
}

int sqpOperatorCode(char *oper)
{
    char *tmp, *ptr;

    /* Convert to lower case */
    tmp = strdup(oper);
    ptr = tmp;
    while (*ptr) {
	*ptr = tolower(*ptr);
	ptr++;
    }

    if (strcmp(oper, "=") == 0)
	return SQLP_EQ;
    else if (strcmp(oper, "<") == 0)
	return SQLP_LT;
    else if (strcmp(oper, "<=") == 0)
	return SQLP_LE;
    else if (strcmp(oper, ">") == 0)
	return SQLP_GT;
    else if (strcmp(oper, ">=") == 0)
	return SQLP_GE;
    else if (strcmp(oper, "<>") == 0)
	return SQLP_NE;
    else if (strcmp(oper, "~") == 0)
	return SQLP_MTCH;
    else if (strcmp(oper, "+") == 0)
	return SQLP_ADD;
    else if (strcmp(oper, "-") == 0)
	return SQLP_SUBTR;
    else if (strcmp(oper, "*") == 0)
	return SQLP_MLTP;
    else if (strcmp(oper, "/") == 0)
	return SQLP_DIV;
    else if (strcmp(oper, "and") == 0)
	return SQLP_AND;
    else if (strcmp(oper, "or") == 0)
	return SQLP_OR;
    else if (strcmp(oper, "not") == 0)
	return SQLP_NOT;

    free(tmp);

    return 0;
}

char *sqpOperatorName(int oper)
{
    switch (oper) {
    case SQLP_EQ:
	return "=";
	break;
    case SQLP_LT:
	return "<";
	break;
    case SQLP_LE:
	return "<=";
	break;
    case SQLP_GT:
	return ">";
	break;
    case SQLP_GE:
	return ">=";
	break;
    case SQLP_NE:
	return "<>";
	break;
    case SQLP_MTCH:
	return "~";
	break;
    case SQLP_ADD:
	return "+";
	break;
    case SQLP_SUBTR:
	return "-";
	break;
    case SQLP_MLTP:
	return "*";
	break;
    case SQLP_DIV:
	return "/";
	break;
    case SQLP_AND:
	return "AND";
	break;
    case SQLP_OR:
	return "OR";
	break;
    case SQLP_NOT:
	return "NOT";
	break;
    }
    return "?";
}
