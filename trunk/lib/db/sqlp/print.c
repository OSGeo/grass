
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

#include <grass/sqlp.h>
#include <stdio.h>

static void print_node(SQLPNODE * nptr, int level)
{
    int i;

    for (i = 0; i < level; i++) {
	fprintf(stderr, "  ");
    }

    if (nptr->node_type == SQLP_NODE_EXPRESSION) {
	fprintf(stderr, "op: %s\n", sqpOperatorName(nptr->oper));
	if (nptr->left) {
	    print_node(nptr->left, level + 1);
	}
	if (nptr->right) {
	    print_node(nptr->right, level + 1);
	}
    }
    else if (nptr->node_type == SQLP_NODE_VALUE) {
	switch (nptr->value.type) {
	case SQLP_NULL:
	    fprintf(stderr, "val: NULL\n");
	    break;
	case SQLP_D:
	    fprintf(stderr, "val: %e\n", nptr->value.d);
	    break;
	case SQLP_I:
	    fprintf(stderr, "val: %d\n", nptr->value.i);
	    break;
	case SQLP_S:
	    fprintf(stderr, "val: '%s'\n", nptr->value.s);
	    break;
	}
    }
    else {			/* SQLP_NODE_COLUMN */
	fprintf(stderr, "col: %s\n", nptr->column_name);
    }
}

int sqpPrintStmt(SQLPSTMT * st)
{
    int i;

    fprintf(stderr, "********** SQL PARSER RESULT **********\n");
    fprintf(stderr, "INPUT: %s\n", sqlpStmt->stmt);
    fprintf(stderr, "COMMAND: ");
    switch (sqlpStmt->command) {
    case (SQLP_ADD_COLUMN):
	fprintf(stderr, "ADD COLUMN\n");
	break;
    case (SQLP_CREATE):
	fprintf(stderr, "CREATE\n");
	break;
    case (SQLP_DROP):
	fprintf(stderr, "DROP\n");
	break;
    case (SQLP_DROP_COLUMN):
	fprintf(stderr, "DROP COLUMN\n");
	break;
    case (SQLP_INSERT):
	fprintf(stderr, "INSERT\n");
	break;
    case (SQLP_UPDATE):
	fprintf(stderr, "UPDATE\n");
	break;
    case (SQLP_SELECT):
	fprintf(stderr, "SELECT\n");
	break;
    case (SQLP_DELETE):
	fprintf(stderr, "DELETE\n");
	break;
    default:
	fprintf(stderr, "UNKNOWN\n");
    }

    fprintf(stderr, "TABLE: %s\n", sqlpStmt->table);

    /* columns */
    for (i = 0; i < st->nCol; i++) {
	if (sqlpStmt->command == SQLP_CREATE) {
	    fprintf(stderr, "COLUMN %2d: ", i + 1);
	    switch (sqlpStmt->ColType[i]) {
	    case (SQLP_VARCHAR):
		fprintf(stderr, "type:varchar width:%d",
			sqlpStmt->ColWidth[i]);
		break;
	    case (SQLP_INTEGER):
		fprintf(stderr, "type:integer");
		break;
	    case (SQLP_DOUBLE):
		fprintf(stderr, "type:double");
		break;
	    case (SQLP_DATE):
		fprintf(stderr, "type:date");
		break;
	    case (SQLP_TIME):
		fprintf(stderr, "type:time");
		break;
	    default:
		fprintf(stderr, "type:unknown");
		break;
	    }
	    fprintf(stderr, " name:%s\n", sqlpStmt->Col[i].s);
	}
	else {
	    fprintf(stderr, "COLUMN %2d: %s\n", i + 1, sqlpStmt->Col[i].s);
	}
    }

    /* values */
    for (i = 0; i < st->nVal; i++) {
	fprintf(stderr, "VALUE %2d ", i + 1);
	switch (sqlpStmt->Val[i].type) {
	case (SQLP_S):
	    fprintf(stderr, "(string) : %s\n", sqlpStmt->Val[i].s);
	    break;
	case (SQLP_I):
	    fprintf(stderr, "(integer): %d\n", sqlpStmt->Val[i].i);
	    break;
	case (SQLP_D):
	    fprintf(stderr, "(float)  : %f\n", sqlpStmt->Val[i].d);
	    break;
	case (SQLP_NULL):
	    fprintf(stderr, "(unknown) : null\n");
	    break;
	case (SQLP_EXPR):
	    fprintf(stderr, "(expression) :\n");
	    print_node(sqlpStmt->Val[i].expr, 0);
	    break;
	default:
	    fprintf(stderr, "unknown\n");
	    break;
	}
    }

    if (sqlpStmt->upperNodeptr) {
	fprintf(stderr, "WHERE:\n");
	print_node(sqlpStmt->upperNodeptr, 0);
    }


    if (sqlpStmt->command == SQLP_SELECT) {
	if (sqlpStmt->orderDir) {
	    fprintf(stderr, "ORDER BY: %s %s\n", sqlpStmt->orderCol,
		    sqlpStmt->orderDir == 1 ? "ASC" : "DESC");
	}
	else {
	    fprintf(stderr, "ORDER BY: %s\n", sqlpStmt->orderCol);
	}
    }


    fprintf(stderr, "***************************************\n");

    return (1);
}
