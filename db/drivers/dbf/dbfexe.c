
/*****************************************************************************
*
* MODULE:       DBF driver 
*   	    	
* AUTHOR(S):    Radim Blazek, Daniel Calvelo
*
* PURPOSE:      Simple driver for reading and writing dbf files     
*
* COPYRIGHT:    (C) 2000,2005 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
* DBF API:      http://shapelib.maptools.org/dbf_api.html
*****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <grass/dbmi.h>
#include <grass/shapefil.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

/* Results of eval_node */
#define NODE_FALSE  0
#define NODE_TRUE   1
#define NODE_VALUE  2
#define NODE_NULL   3
#define NODE_ERROR  4

int yyparse(void);
void get_col_def(SQLPSTMT * st, int col, int *type, int *width,
		 int *decimals);
int sel(SQLPSTMT * st, int tab, int **set);
void eval_val(int tab, int row, int col, SQLPVALUE * inval,
	      SQLPVALUE * result);
int set_val(int tab, int row, int col, SQLPVALUE * val);
double eval_node(SQLPNODE *, int, int, SQLPVALUE *);
int eval_node_type(SQLPNODE *, int);

int execute(char *sql, cursor * c)
{
    int i, j, tab, ret;
    SQLPSTMT *st;
    ROW *dbrows;
    VALUE *dbval;
    int row, nrows;
    int *cols = NULL, ncols, col;
    int *selset;
    int dtype, stype;
    int width, decimals;
    char *tmpsql, name[500];
    SQLPVALUE *calctmp;		/* store for calculated values in UPDATE, if any */

    /* parse sql statement */
    /* I don't know why, but if the statement ends by string in quotes 'xxx' and is not 
     *  followed by space or '\n' it is not parsed properly -> */
    tmpsql = (char *)G_malloc(strlen(sql) + 2);
    sprintf(tmpsql, "%s ", sql);
    st = sqpInitStmt();
    st->stmt = tmpsql;
    sqpInitParser(st);

    if (yyparse() != 0) {
	G_free(tmpsql);
	db_d_append_error("%s (%s) %s\n%s\n",
			  _("SQL parser error"),
			  st->errmsg,
			  _("in statement:"),
			  sql);
	sqpFreeStmt(st);
	return DB_FAILED;
    }
    G_free(tmpsql);

    G_debug(3, "SQL statement parsed successfully: %s", sql);

    /* sqpPrintStmt(st); *//* debug output only */

    /* find table */
    tab = find_table(st->table);
    if (tab < 0 && st->command != SQLP_CREATE) {
	db_d_append_error(_("Table '%s' doesn't exist."), st->table);
	return DB_FAILED;
    }

    /* For DROP we have to call load_table_head() because it reads permissions */
    if ((st->command != SQLP_CREATE)) {
	ret = load_table_head(tab);
	if (ret == DB_FAILED) {
	    db_d_append_error(_("Unable to load table head."));
	    return DB_FAILED;
	}
    }

    if ((st->command == SQLP_DROP) || (st->command == SQLP_DELETE) ||
	(st->command == SQLP_INSERT) || (st->command == SQLP_UPDATE) ||
	(st->command == SQLP_ADD_COLUMN) || (st->command == SQLP_DROP_COLUMN)
	) {
	if (db.tables[tab].write == FALSE) {
	    db_d_append_error(_("Unable to modify table, "
				"don't have write permission for DBF file."));
	    return DB_FAILED;
	}
    }

    /* find columns */
    ncols = st->nCol;
    if (st->command == SQLP_INSERT || st->command == SQLP_SELECT
	|| st->command == SQLP_UPDATE || st->command == SQLP_DROP_COLUMN) {
	if (ncols > 0) {	/* columns were specified */
	    cols = (int *)G_malloc(ncols * sizeof(int));
	    for (i = 0; i < ncols; i++) {
		cols[i] = find_column(tab, st->Col[i].s);
		if (cols[i] == -1) {
		    db_d_append_error(_("Column '%s' not found"), st->Col[i].s);
		    return DB_FAILED;
		}
	    }
	}
	else {			/* all columns */

	    ncols = db.tables[tab].ncols;
	    cols = (int *)G_malloc(ncols * sizeof(int));
	    for (i = 0; i < ncols; i++)
		cols[i] = i;
	}
    }

    /* check column types */
    if (st->command == SQLP_INSERT || st->command == SQLP_UPDATE) {
	for (i = 0; i < st->nVal; i++) {
	    col = cols[i];
	    if (st->Val[i].type != SQLP_NULL && st->Val[i].type != SQLP_EXPR) {
		dtype = db.tables[tab].cols[col].type;
		stype = st->Val[i].type;
		if ((dtype == DBF_INT && stype != SQLP_I)
		    || (dtype == DBF_DOUBLE && stype == SQLP_S)
		    || (dtype == DBF_CHAR && stype != SQLP_S)) {
		    db_d_append_error(_("Incompatible value type."));
		    return DB_FAILED;
		}
	    }
	}
    }

    /* do command */
    G_debug(3, "Doing SQL command <%d> on DBF table... (see include/sqlp.h)",
	    st->command);
    switch (st->command) {
    case (SQLP_ADD_COLUMN):
	load_table(tab);
	get_col_def(st, 0, &dtype, &width, &decimals);
	ret = add_column(tab, dtype, st->Col[0].s, width, decimals);
	if (ret == DB_FAILED) {
	    db_d_append_error(_("Unable to add column."));
	    return DB_FAILED;
	}
	/* Add column to each row */
	for (i = 0; i < db.tables[tab].nrows; i++) {
	    db.tables[tab].rows[i].values =
		(VALUE *) G_realloc(db.tables[tab].rows[i].values,
				    db.tables[tab].ncols * sizeof(VALUE));

	    dbval =
		&(db.tables[tab].rows[i].values[db.tables[tab].ncols - 1]);
	    dbval->i = 0;
	    dbval->d = 0.0;
	    dbval->c = NULL;
	    dbval->is_null = 1;
	}
	db.tables[tab].updated = TRUE;
	break;

    case (SQLP_DROP_COLUMN):
	load_table(tab);
	if (drop_column(tab, st->Col[0].s) != DB_OK) {
	    db_d_append_error(_("Unable to delete column."));
	    return DB_FAILED;
	}
	db.tables[tab].updated = TRUE;
	break;

    case (SQLP_CREATE):
	if (tab >= 0) {
	    db_d_append_error(_("Table %s already exists"), st->table);
	    
	    return DB_FAILED;
	}
	sprintf(name, "%s.dbf", st->table);
	add_table(st->table, name);

	tab = find_table(st->table);
	db.tables[tab].read = TRUE;
	db.tables[tab].write = TRUE;

	for (i = 0; i < ncols; i++) {
	    get_col_def(st, i, &dtype, &width, &decimals);
	    ret = add_column(tab, dtype, st->Col[i].s, width, decimals);
	    if (ret == DB_FAILED) {
		db_d_append_error(_("Unable to create table."));
		db.tables[tab].alive = FALSE;
		return DB_FAILED;
	    }
	}
	db.tables[tab].described = TRUE;
	db.tables[tab].loaded = TRUE;
	db.tables[tab].updated = TRUE;
	break;

    case (SQLP_DROP):
	unlink(db.tables[tab].file);
	db.tables[tab].alive = FALSE;
	break;

    case (SQLP_INSERT):
	load_table(tab);

	/* add row */
	if (db.tables[tab].nrows == db.tables[tab].arows) {
	    db.tables[tab].arows += 1000;
	    db.tables[tab].rows =
		(ROW *) G_realloc(db.tables[tab].rows,
				  db.tables[tab].arows * sizeof(ROW));
	}
	dbrows = db.tables[tab].rows;
	row = db.tables[tab].nrows;
	dbrows[row].values =
	    (VALUE *) G_calloc(db.tables[tab].ncols, sizeof(VALUE));
	dbrows[row].alive = TRUE;

	/* set to null */
	for (i = 0; i < db.tables[tab].ncols; i++) {
	    VALUE *dbval;

	    dbval = &(dbrows[row].values[i]);
	    dbval->is_null = 1;
	}

	/* set values */
	for (i = 0; i < st->nVal; i++) {
	    col = cols[i];
	    set_val(tab, row, col, &(st->Val[i]));
	}

	db.tables[tab].nrows++;
	db.tables[tab].updated = TRUE;
	break;

    case (SQLP_SELECT):
	G_debug(2, "SELECT");
	c->st = st;
	c->table = tab;
	c->cols = cols;
	c->ncols = ncols;
	c->nrows = sel(st, tab, &(c->set));
	if (c->nrows < 0) {
	    db_d_append_error(_("Error in selecting rows"));
	    return DB_FAILED;
	}
	c->cur = -1;

	break;

    case (SQLP_UPDATE):
	nrows = sel(st, tab, &selset);
	if (nrows < 0) {
	    db_d_append_error(_("Error in selecting rows"));
	    return DB_FAILED;
	}
	dbrows = db.tables[tab].rows;

	/* update rows */
	for (i = 0; i < nrows; i++) {
	    SQLPVALUE *temp_p;

	    calctmp = (SQLPVALUE *) G_malloc((st->nVal) * sizeof(SQLPVALUE));
	    row = selset[i];
	    for (j = 0; j < st->nVal; j++) {
		col = cols[j];
		eval_val(tab, row, col, &(st->Val[j]), &(calctmp[j]));
	    }
	    temp_p = st->Val;
	    st->Val = calctmp;
	    for (j = 0; j < st->nVal; j++) {
		col = cols[j];
		set_val(tab, row, col, &(st->Val[j]));
		db.tables[tab].updated = TRUE;
	    }
	    st->Val = temp_p;
	    G_free(calctmp);
	}
	break;

    case (SQLP_DELETE):
	nrows = sel(st, tab, &selset);
	if (nrows < 0) {
	    db_d_append_error(_("Error in selecting rows"));
	    return DB_FAILED;
	}
	dbrows = db.tables[tab].rows;

	/* delete rows */
	for (i = 0; i < nrows; i++) {
	    row = selset[i];
	    dbrows[row].alive = FALSE;
	    db.tables[tab].updated = TRUE;
	}
	break;

    }
    if (st->command != SQLP_SELECT) {	/* because statement is released with cursor */
	sqpFreeStmt(st);
	if (cols)
	    G_free(cols);
    }

    return DB_OK;
}

/* for given parser result and column index finds dbf column definition */
void get_col_def(SQLPSTMT * st, int col, int *type, int *width, int *decimals)
{
    switch (st->ColType[col]) {
    case (SQLP_INTEGER):
	*type = DBF_INT;
	*width = 11;
	*decimals = 0;
	break;
    case (SQLP_VARCHAR):
	*type = DBF_CHAR;
	*width = st->ColWidth[col];
	*decimals = 0;
	break;
    case (SQLP_DATE):		/* DATE treated as string unless SHAPELIB/DBFLIB supports date type */
	*type = DBF_CHAR;
	*width = 10;		/* 2004-01-23 = 10 chars */
	*decimals = 0;
	break;
    case (SQLP_DOUBLE):
	*type = DBF_DOUBLE;
	*width = 20;
	*decimals = 6;
	break;
    }
}

void eval_val(int tab, int row, int col, SQLPVALUE * inval, SQLPVALUE * val)
{

    double retval;

    /* XXX */
    if (inval->type == SQLP_EXPR) {

	retval = eval_node(inval->expr, tab, row, val);
	if (retval == NODE_NULL) {
	    val->type = SQLP_NULL;
	}
	else if (retval == NODE_TRUE) {
	    val->i = 1;
	    val->d = 1.0;
	    val->s = "TRUE";
	}
	else if (retval == NODE_FALSE) {
	    val->i = 0;
	    val->d = 0.0;
	    val->s = NULL;
	}
	else if (retval == NODE_VALUE) {
	    /* Ok, got a value, propagate it to the proper type */
	    if (val->type == SQLP_I) {
		val->d = (double)val->i;
		val->s = (char *)G_malloc(32 * sizeof(char));
		sprintf(val->s, "%d", val->i);
	    }
	    else if (val->type == SQLP_D) {
		val->i = (int)val->d;
		val->s = (char *)G_malloc(32 * sizeof(char));
		sprintf(val->s, "%g", val->d);
	    }
	    else if (val->type == SQLP_S) {
		val->i = atoi(val->s);
		val->d = atof(val->s);
	    }
	    else {
		G_fatal_error
		    ("This should not happen: wrong return type in parsing.");
	    }
	}
	else if (retval == NODE_ERROR) {
	    G_fatal_error
		("This should not happen: got a wrong expression structure after parsing.");
	}
	else {
	    G_fatal_error
		("Unknown return value calling eval_node from eval_val");
	}
    }
    else {
	/* 
	 * TODO: maybe use this function to perform type "conversion",
	 * i.e. setting all of s,i,d to the same "value",as is done with
	 * the result of eval_node above.
	 */
	val = inval;
    }
}

int set_val(int tab, int row, int col, SQLPVALUE * val)
{
    VALUE *dbval;

    dbval = &(db.tables[tab].rows[row].values[col]);
    /* For debugging purposes; see FIXME below      
       fprintf(stderr, "In set_val : ");
       fprintf(stderr, val->type==SQLP_EXPR?"sqlp_expr":
       val->type==SQLP_NULL?"sqlp_null":
       val->type==SQLP_I?"sqlp_i":
       val->type==SQLP_D?"sqlp_d":
       val->type==SQLP_S?"sqlp_s":
       "other"); //DCA
       fprintf(stderr,"%d\n",val->type);
       fflush(stderr);
     */
    if (val->type == SQLP_EXPR) {
	eval_val(tab, row, col, val, val);
    }

    /* FIXME: SQLP_NULL is not always properly detected.
     * This workaround works, since type should be some of these
     * after passing through eval_val; otherwise it is NULL
     */
    if (!(val->type == SQLP_I || val->type == SQLP_D || val->type == SQLP_S)) {
	dbval->is_null = 1;
	dbval->c = NULL;
	dbval->i = 0;
	dbval->d = 0.0;
    }
    else {
	dbval->is_null = 0;
	switch (db.tables[tab].cols[col].type) {
	case DBF_INT:
	    dbval->i = val->i;
	    break;
	case DBF_CHAR:
	    save_string(dbval, val->s);
	    break;
	case DBF_DOUBLE:
	    if (val->type == SQLP_I)
		dbval->d = val->i;
	    else if (val->type == SQLP_D)
		dbval->d = val->d;
	    else if (val->type == SQLP_S) {
		char *tailptr;
		double dval = strtod(val->s, &tailptr);

		if (!(*tailptr)) {
		    dbval->d = dval;
		}
	    }
	    break;
	}
    }
    return (1);
}

/* Comparison of 2 rows */
static int cur_cmp_table;
static int cur_cmp_ocol;
static int cmp_row_asc(const void *pa, const void *pb)
{
    int *row1 = (int *)pa;
    int *row2 = (int *)pb;
    char *c1, *c2;
    int i1, i2;
    double d1, d2;
    TABLE *tbl;

    tbl = &(db.tables[cur_cmp_table]);

    if (tbl->rows[*row1].values[cur_cmp_ocol].is_null) {
	if (tbl->rows[*row2].values[cur_cmp_ocol].is_null) {
	    return 0;
	}
	else {
	    return 1;
	}
    }
    else {
	if (tbl->rows[*row2].values[cur_cmp_ocol].is_null) {
	    return -1;
	}
	else {
	    switch (tbl->cols[cur_cmp_ocol].type) {
	    case DBF_CHAR:
		c1 = tbl->rows[*row1].values[cur_cmp_ocol].c;
		c2 = tbl->rows[*row2].values[cur_cmp_ocol].c;
		return (strcmp(c1, c2));
		break;
	    case DBF_INT:
		i1 = tbl->rows[*row1].values[cur_cmp_ocol].i;
		i2 = tbl->rows[*row2].values[cur_cmp_ocol].i;
		if (i1 < i2)
		    return -1;
		if (i1 > i2)
		    return 1;
		return 0;
		break;
	    case DBF_DOUBLE:
		d1 = tbl->rows[*row1].values[cur_cmp_ocol].d;
		d2 = tbl->rows[*row2].values[cur_cmp_ocol].d;
		if (d1 < d2)
		    return -1;
		if (d1 > d2)
		    return 1;
		return 0;
		break;
	    }
	    return 0;
	}
    }
}

static int cmp_row_desc(const void *pa, const void *pb)
{

    return -cmp_row_asc(pa, pb);

}

/* Select records, sets 'selset' to new array of items and returns
 *  number of items or -1 for error */
int sel(SQLPSTMT * st, int tab, int **selset)
{
    int i, ret, condition;
    int *set;			/* pointer to array of indexes to rows */
    int aset, nset;

    G_debug(2, "sel(): tab = %d", tab);

    *selset = NULL;
    nset = 0;

    ret = load_table(tab);
    if (ret == DB_FAILED) {
	db_d_append_error(_("Cannot load table."));
	return -1;
    }

    aset = 1;
    set = (int *)G_malloc(aset * sizeof(int));

    if (st->upperNodeptr) {
	int node_type;

	/* First eval node type */
	node_type = eval_node_type(st->upperNodeptr, tab);
	G_debug(4, "node result type = %d", node_type);

	if (node_type == -1) {
	    db_d_append_error(_("Incompatible types in WHERE condition."));
	    return -1;
	}
	else if (node_type == SQLP_S || node_type == SQLP_I ||
		 node_type == SQLP_D) {
	    db_d_append_error(_("Result of WHERE condition is not of type BOOL."));
	    return -1;
	}
	else if (node_type == SQLP_NULL) {
	    /* Conditions has undefined result -> nothing selected */
	    return 0;
	}
	else if (node_type == SQLP_BOOL) {
	    for (i = 0; i < db.tables[tab].nrows; i++) {
		SQLPVALUE value;

		G_debug(4, "row %d", i);
		condition = eval_node(st->upperNodeptr, tab, i, &value);
		G_debug(4, "condition = %d", condition);

		if (condition == NODE_ERROR) {	/* e.g. division by 0 */
		    db_d_append_error(_("Error in evaluation of WHERE condition."));
		    return -1;
		}
		else if (condition == NODE_TRUE) {	/* true */
		    if (nset == aset) {
			aset += 1000;
			set = (int *)G_realloc(set, aset * sizeof(int));
		    }
		    set[nset] = i;
		    nset++;
		}
		else if (condition != NODE_FALSE && condition != NODE_NULL) {	/* Should not happen */
		    db_d_append_error(_("Unknown result (%d) of WHERE evaluation"),
				      condition);
		    return -1;
		}
	    }
	}
	else {			/* Should not happen */
	    db_d_append_error(_("Unknown WHERE condition type (bug in DBF driver)."));
	    return -1;
	}
    }
    else {			/* Select all */
	aset = db.tables[tab].nrows;
	set = (int *)G_realloc(set, aset * sizeof(int));
	for (i = 0; i < db.tables[tab].nrows; i++) {
	    set[i] = i;
	}
	nset = db.tables[tab].nrows;
    }

    /* Order */
    if (st->command == SQLP_SELECT && st->orderCol) {
	G_debug(3, "Order selection by %s", st->orderCol);

	/* Find order col */
	cur_cmp_ocol = -1;
	for (i = 0; i < db.tables[tab].ncols; i++) {
	    if (strcmp(db.tables[tab].cols[i].name, st->orderCol) == 0) {
		cur_cmp_ocol = i;
		break;
	    }
	}
	if (cur_cmp_ocol < 0) {
	    db_d_append_error(_("Unable to find order column '%s'"), st->orderCol);
	    return -1;
	}

	cur_cmp_table = tab;
	if (st->orderDir == SORT_DESC) {
	    qsort(set, nset, sizeof(int), cmp_row_desc);
	}
	else {
	    qsort(set, nset, sizeof(int), cmp_row_asc);
	}


    }

    *selset = set;
    return nset;
}

/* Evaluate node recursively.
 *
 * Returns: 
 *    NODE_NULL  result/value is unknown   
 *    NODE_TRUE   
 *    NODE_FALSE  
 *    NODE_VALUE result is a value stored in 'value' 
 *               (if value is not NULL otherwise NODE_NULL is returned and value is not set)
 *    NODE_ERROR e.g. division by 0
 *
 * If results is NODE_VALUE, the 'value' is set, if value is type SQLP_S the string is not duplicated
 * and only pointer is set -> do not free value->s 
 */
double eval_node(SQLPNODE * nptr, int tab, int row, SQLPVALUE * value)
{
    int left, right;
    SQLPVALUE left_value, right_value;
    int ccol;
    COLUMN *col;
    VALUE *val;
    double left_dval, right_dval, dval;
    char *rightbuf;

    /* Note: node types were previously checked by eval_node_type */

    G_debug(4, "eval_node node_type = %d", nptr->node_type);

    switch (nptr->node_type) {
    case SQLP_NODE_VALUE:
	if (nptr->value.type == SQLP_NULL)
	    return NODE_NULL;

	value->type = nptr->value.type;
	value->s = nptr->value.s;
	value->i = nptr->value.i;
	value->d = nptr->value.d;
	return NODE_VALUE;
	break;

    case SQLP_NODE_COLUMN:
	ccol = find_column(tab, nptr->column_name);
	col = &(db.tables[tab].cols[ccol]);
	val = &(db.tables[tab].rows[row].values[ccol]);

	if (val->is_null)
	    return NODE_NULL;

	switch (col->type) {
	case DBF_CHAR:
	    value->s = val->c;
	    value->type = SQLP_S;
	    break;
	case DBF_INT:
	    value->i = val->i;
	    value->type = SQLP_I;
	    break;
	case DBF_DOUBLE:
	    value->d = val->d;
	    value->type = SQLP_D;
	    break;
	}
	return NODE_VALUE;
	break;

    case SQLP_NODE_EXPRESSION:
	/* Note: Some expressions (e.g. NOT) have only one side */
	if (nptr->left) {
	    left = eval_node(nptr->left, tab, row, &left_value);
	    G_debug(4, "    left = %d", left);

	    if (left == NODE_ERROR)
		return NODE_ERROR;

	    if (left != NODE_NULL) {
		if (left_value.type == SQLP_I)
		    left_dval = left_value.i;
		else
		    left_dval = left_value.d;

		G_debug(4, "    left_dval = %f", left_dval);
	    }
	}

	if (nptr->right) {
	    right = eval_node(nptr->right, tab, row, &right_value);
	    G_debug(4, "    right = %d", right);

	    if (right == NODE_ERROR)
		return NODE_ERROR;

	    if (right != NODE_NULL) {
		if (right_value.type == SQLP_I)
		    right_dval = right_value.i;
		else
		    right_dval = right_value.d;

		G_debug(4, "    right_dval = %f", right_dval);
	    }
	}

	G_debug(4, "    operator = %d", nptr->oper);

	switch (nptr->oper) {
	    /* Arithmetical */
	case SQLP_ADD:
	case SQLP_SUBTR:
	case SQLP_MLTP:
	case SQLP_DIV:
	    if (left == NODE_NULL || right == NODE_NULL)
		return NODE_NULL;

	    switch (nptr->oper) {
	    case SQLP_ADD:
		dval = left_dval + right_dval;
		break;
	    case SQLP_SUBTR:
		dval = left_dval - right_dval;
		break;
	    case SQLP_MLTP:
		dval = left_dval * right_dval;
		break;
	    case SQLP_DIV:
		if (right_dval != 0.0) {
		    dval = left_dval / right_dval;
		}
		else {
		    db_d_append_error(_("Division by zero"));
		    return NODE_ERROR;
		}
		break;
	    }

	    if (left_value.type == SQLP_I && right_value.type == SQLP_I &&
		(nptr->oper == SQLP_ADD || nptr->oper == SQLP_SUBTR ||
		 nptr->oper == SQLP_MLTP)) {
		value->type = SQLP_I;
		value->i = (int)dval;
	    }
	    else {
		value->type = SQLP_D;
		value->d = dval;
	    }
	    return NODE_VALUE;

	    break;

	    /* Comparison */
	    /* Operators valid for all type */
	case SQLP_EQ:
	    if (left == NODE_NULL || right == NODE_NULL) {
		return NODE_NULL;
	    }
	    else if (left_value.type == SQLP_S) {	/* we checked before if right is also string */
		if (left_value.s && right_value.s &&
		    strcmp(left_value.s, right_value.s) == 0)
		    return NODE_TRUE;
		else
		    return NODE_FALSE;
	    }
	    else {		/* numbers */
		if (left_dval == right_dval)
		    return NODE_TRUE;
		else
		    return NODE_FALSE;
	    }
	    break;

	case SQLP_NE:
	    if (left == NODE_NULL || right == NODE_NULL) {
		return NODE_NULL;
	    }
	    else if (left_value.type == SQLP_S) {	/* we checked before if right is also string */
		if (left_value.s && right_value.s &&
		    strcmp(left_value.s, right_value.s) != 0)
		    return NODE_TRUE;
		else
		    return NODE_FALSE;
	    }
	    else {		/* numbers */
		if (left_dval != right_dval)
		    return NODE_TRUE;
		else
		    return NODE_FALSE;
	    }

	    /* Operators valid for numbers */
	case SQLP_LT:
	    if (left == NODE_NULL || right == NODE_NULL) {
		return NODE_NULL;
	    }
	    else {
		if (left_dval < right_dval)
		    return NODE_TRUE;
		else
		    return NODE_FALSE;
	    }

	case SQLP_LE:
	    if (left == NODE_NULL || right == NODE_NULL) {
		return NODE_NULL;
	    }
	    else {
		if (left_dval <= right_dval)
		    return NODE_TRUE;
		else
		    return NODE_FALSE;
	    }

	case SQLP_GT:
	    if (left == NODE_NULL || right == NODE_NULL) {
		return NODE_NULL;
	    }
	    else {
		if (left_dval > right_dval)
		    return NODE_TRUE;
		else
		    return NODE_FALSE;
	    }

	case SQLP_GE:
	    if (left == NODE_NULL || right == NODE_NULL) {
		return NODE_NULL;
	    }
	    else {
		if (left_dval >= right_dval)
		    return NODE_TRUE;
		else
		    return NODE_FALSE;
	    }

	    /* Operator valid for string */
	case SQLP_MTCH:
	    if (left == NODE_NULL || right == NODE_NULL) {
		return NODE_NULL;
	    }
	    else {
		/* hack to get '%substring' and 'substring%' working */
		rightbuf = G_str_replace(right_value.s, "%", "");
		G_chop(rightbuf);
		if (left_value.s && right_value.s &&
		    strstr(left_value.s, rightbuf) != NULL) {
		    G_free(rightbuf);
		    return NODE_TRUE;
		}
		else {
		    G_free(rightbuf);
		    return NODE_FALSE;
		}
	    }

	case SQLP_ISNULL:
	    return right == NODE_NULL ? NODE_TRUE : NODE_FALSE;

	case SQLP_NOTNULL:
	    return right != NODE_NULL ? NODE_TRUE : NODE_FALSE;

	    /* Logical */
	case SQLP_AND:
	    if (left == NODE_NULL || right == NODE_NULL) {
		return NODE_NULL;
	    }
	    else if (left == NODE_TRUE && right == NODE_TRUE) {
		return NODE_TRUE;
	    }
	    else if (left == NODE_VALUE || right == NODE_VALUE) {	/* Should not happen */
		db_d_append_error(_("Value operand for AND"));
		return NODE_ERROR;
	    }
	    else {
		return NODE_FALSE;
	    }
	case SQLP_OR:
	    if (left == NODE_NULL && right == NODE_NULL) {
		return NODE_NULL;
	    }
	    else if (left == NODE_TRUE || right == NODE_TRUE) {
		return NODE_TRUE;
	    }
	    else if (left == NODE_VALUE || right == NODE_VALUE) {	/* Should not happen */
		db_d_append_error(_("Value operand for OR"));
		return NODE_ERROR;
	    }
	    else {
		return NODE_FALSE;
	    }
	case SQLP_NOT:
	    /* sub node stored on the right side */
	    if (right == NODE_NULL) {
		return NODE_NULL;
	    }
	    else if (right == NODE_TRUE) {
		return NODE_FALSE;
	    }
	    else if (right == NODE_VALUE) {	/* Should not happen */
		db_d_append_error(_("Value operand for NOT"));
		return NODE_ERROR;
	    }
	    else {
		return NODE_TRUE;
	    }

	default:
	    db_d_append_error(_("Unknown operator %d"), nptr->oper);
	    return NODE_FALSE;
	}
    }

    return NODE_ERROR;		/* Not reached */
}

/* Recursively get value/expression type.
 * Returns: node type (SQLP_S, SQLP_I, SQLP_D, SQLP_NULL, SQLP_BOOL)
 *          -1 on error (if types in expression are not compatible) 
 *
 * Rules:
 *      Values (in SQL Statement):
 *        SQLP_S              -> SQLP_S
 *        SQLP_I              -> SQLP_I
 *        SQLP_D              -> SQLP_D
 *        SQLP_NULL           -> SQLP_NULL
 *      Columns (in dbf table):
 *        DBF_CHAR            -> SQLP_S
 *        DBF_INT             -> SQLP_I
 *        DBF_DOUBLE          -> SQLP_D
 *      Arithetical Expressions :
 *        side1   side2           exp  
 *        SQLP_S    ALL           ALL    -> error
 *        SQLP_NULL SQLP_I        ALL    -> SQLP_NULL
 *        SQLP_NULL SQLP_D        ALL    -> SQLP_NULL
 *        SQLP_I    SQLP_I        +,-,*  -> SQLP_I
 *        SQLP_I    SQLP_I        /      -> SQLP_D
 *        SQLP_I    SQLP_D        ALL    -> SQLP_D
 *        SQLP_D    SQLP_D        ALL    -> SQLP_D
 *      Comparisons :
 *        side1     side2     exp  
 *        SQLP_S    SQLP_S    =,<>,~          -> SQLP_BOOL
 *        SQLP_S    SQLP_S    <,<=,>,>=       -> error
 *        SQLP_S    SQLP_I    ALL             -> error
 *        SQLP_S    SQLP_D    ALL             -> error
 *        SQLP_I    SQLP_I    =,<>,<,<=,>,>=  -> SQLP_BOOL
 *        SQLP_D    SQLP_D    =,<>,<,<=,>,>=  -> SQLP_BOOL
 *        SQLP_I    SQLP_D    =,<>,<,<=,>,>=  -> SQLP_BOOL
 *        SQLP_I    ALL       ~               -> error
 *        SQLP_D    ALL       ~               -> error
 *        SQLP_NULL ALL       ALL             -> SQLP_NULL
 *      Logical expressions 
 *        In general, if we know that the result is NULL regardless actual values it returns SQLP_NULL
 *        so that tests for individual rows are not performed, otherwise SQLP_BOOL
 *        SQLP_BOOL SQLP_BOOL AND               -> SQLP_BOOL
 *        SQLP_BOOL SQLP_NULL AND               -> SQLP_NULL
 *        SQLP_NULL SQLP_NULL AND               -> SQLP_NULL
 *        SQLP_BOOL SQLP_BOOL OR                -> SQLP_BOOL
 *        SQLP_BOOL SQLP_NULL OR                -> SQLP_BOOL
 *        SQLP_NULL SQLP_NULL OR                -> SQLP_NULL
 *        SQLP_BOOL -         NOT               -> SQLP_BOOL
 *        SQLP_NULL -         NOT               -> SQLP_NULL
 */
int eval_node_type(SQLPNODE * nptr, int tab)
{
    int left, right;
    int ccol;
    COLUMN *col = NULL;

    switch (nptr->node_type) {
    case SQLP_NODE_VALUE:
	return nptr->value.type;
	break;

    case SQLP_NODE_COLUMN:
	ccol = find_column(tab, nptr->column_name);
	if (ccol == -1) {
	    db_d_append_error(_("Column '%s' not found"), nptr->column_name);
	    return -1;
	}
	col = &(db.tables[tab].cols[ccol]);
	switch (col->type) {
	case DBF_CHAR:
	    return (SQLP_S);
	    break;
	case DBF_INT:
	    return (SQLP_I);
	    break;
	case DBF_DOUBLE:
	    return (SQLP_D);
	    break;
	}
	break;

    case SQLP_NODE_EXPRESSION:
	/* Note: Some expressions (e.g. NOT) have only one side */
	if (nptr->left) {
	    left = eval_node_type(nptr->left, tab);
	    if (left == -1)
		return -1;
	}

	if (nptr->right) {
	    right = eval_node_type(nptr->right, tab);
	    if (right == -1)
		return -1;
	}

	switch (nptr->oper) {
	    /* Arithmetical */
	case SQLP_ADD:
	case SQLP_SUBTR:
	case SQLP_MLTP:
	case SQLP_DIV:
	    if (left == SQLP_S || right == SQLP_S) {
		db_d_append_error(_("Arithmetical operation with strings is not allowed"));
		return -1;
	    }
	    else if (left == SQLP_NULL || right == SQLP_NULL) {
		return SQLP_NULL;
	    }
	    else if (left == SQLP_I && right == SQLP_I &&
		     (nptr->oper == SQLP_ADD || nptr->oper == SQLP_SUBTR ||
		      nptr->oper == SQLP_MLTP)) {
		return SQLP_I;
	    }
	    else {
		return SQLP_D;
	    }
	    break;

	    /* Comparison */
	    /* Operators valid for all type */
	case SQLP_EQ:
	case SQLP_NE:
	    if ((left == SQLP_S && (right == SQLP_I || right == SQLP_D)) ||
		(right == SQLP_S && (left == SQLP_I || left == SQLP_D))) {
		db_d_append_error(_("Comparison between string and number is not allowed"));
		return -1;
	    }
	    else if (left == SQLP_NULL || right == SQLP_NULL) {
		return SQLP_NULL;
	    }
	    else {
		return SQLP_BOOL;
	    }
	    /* Operators valid for numbers */
	case SQLP_LT:
	case SQLP_LE:
	case SQLP_GT:
	case SQLP_GE:
	    if (left == SQLP_S || right == SQLP_S) {
		db_d_append_error(_("Comparison '%s' between strings not allowed"),
				  sqpOperatorName(nptr->oper));
		return -1;
	    }
	    else if (left == SQLP_NULL || right == SQLP_NULL) {
		return SQLP_NULL;
	    }
	    else {
		return SQLP_BOOL;
	    }
	    /* Operator valid for string */
	case SQLP_MTCH:
	    if (left == SQLP_I || left == SQLP_D || right == SQLP_I ||
		right == SQLP_D) {
		db_d_append_error(_("Match (~) between numbers not allowed"));
		return -1;
	    }
	    else if (left == SQLP_NULL || right == SQLP_NULL) {
		return SQLP_NULL;
	    }
	    else {
		return SQLP_BOOL;
	    }

	case SQLP_ISNULL:
	case SQLP_NOTNULL:
	    return SQLP_BOOL;

	    /* Logical */
	case SQLP_AND:
	    if (left == SQLP_NULL || right == SQLP_NULL) {
		return SQLP_NULL;
	    }
	    else {
		return SQLP_BOOL;
	    }
	case SQLP_OR:
	    if (left == SQLP_NULL && right == SQLP_NULL) {
		return SQLP_NULL;
	    }
	    else {
		return SQLP_BOOL;
	    }
	case SQLP_NOT:
	    /* sub node stored on the right side */
	    if (right == SQLP_NULL) {
		return SQLP_NULL;
	    }
	    else {
		return SQLP_BOOL;
	    }

	default:
	    db_d_append_error(_("Unknown operator %d"), nptr->oper);
	    return -1;
	}
    }

    return -1;			/* Not reached */
}
