/*!
 * \file db/dbmi_client/select.c
 * 
 * \brief DBMI Library (client) - select records from table
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

static int cmp(const void *pa, const void *pb)
{
    int *p1 = (int *)pa;
    int *p2 = (int *)pb;

    if (*p1 < *p2)
	return -1;
    if (*p1 > *p2)
	return 1;
    return 0;
}

static int cmpcat(const void *pa, const void *pb)
{
    dbCatVal *p1 = (dbCatVal *) pa;
    dbCatVal *p2 = (dbCatVal *) pb;

    if (p1->cat < p2->cat)
	return -1;
    if (p1->cat > p2->cat)
	return 1;
    return 0;
}

static int cmpcatkey(const void *pa, const void *pb)
{
    int *p1 = (int *)pa;
    dbCatVal *p2 = (dbCatVal *) pb;

    if (*p1 < p2->cat)
	return -1;
    if (*p1 > p2->cat)
	return 1;
    return 0;
}

static int cmpvalueint(const void *pa, const void *pb)
{
    dbCatVal *p1 = (dbCatVal *) pa;
    dbCatVal *p2 = (dbCatVal *) pb;

    if (p1->val.i < p2->val.i)
	return -1;
    if (p1->val.i > p2->val.i)
	return 1;

    return 0;
}

static int cmpvaluedouble(const void *pa, const void *pb)
{
    dbCatVal *p1 = (dbCatVal *) pa;
    dbCatVal *p2 = (dbCatVal *) pb;

    if (p1->val.d < p2->val.d)
	return -1;
    if (p1->val.d > p2->val.d)
	return 1;

    return 0;
}

static int cmpvaluestring(const void *pa, const void *pb)
{
    dbCatVal *const *a = pa;
    dbCatVal *const *b = pb;

    return strcmp((const char *)a, (const char *)b);
}

/*!
  \brief Select array of ordered integers from table/column

  \param driver DB driver
  \param tab table name
  \param col column name
  \param where where statement
  \param[out] pval array of ordered integer values

  \return number of selected values
  \return -1 on error
*/
int db_select_int(dbDriver * driver, const char *tab, const char *col,
		  const char *where, int **pval)
{
    int type, more, alloc, count;
    int *val;
    char *buf = NULL;
    const char *sval;
    dbString stmt;
    dbCursor cursor;
    dbColumn *column;
    dbValue *value;
    dbTable *table;

    G_debug(3, "db_select_int()");

    if (col == NULL || strlen(col) == 0) {
	G_warning(_("Missing column name"));
	return -1;
    }

    /* allocate */
    alloc = 1000;
    val = (int *)G_malloc(alloc * sizeof(int));

    if (where == NULL || strlen(where) == 0)
	G_asprintf(&buf, "SELECT %s FROM %s", col, tab);
    else
	G_asprintf(&buf, "SELECT %s FROM %s WHERE %s", col, tab, where);

    G_debug(3, "  SQL: %s", buf);

    db_init_string(&stmt);
    db_set_string(&stmt, buf);
    G_free(buf);

    if (db_open_select_cursor(driver, &stmt, &cursor, DB_SEQUENTIAL) != DB_OK)
	return (-1);

    table = db_get_cursor_table(&cursor);
    column = db_get_table_column(table, 0);	/* first column */
    if (column == NULL) {
	return -1;
    }
    value = db_get_column_value(column);
    type = db_get_column_sqltype(column);
    type = db_sqltype_to_Ctype(type);

    /* fetch the data */
    count = 0;
    while (1) {
	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
	    return (-1);

	if (!more)
	    break;

	if (count == alloc) {
	    alloc += 1000;
	    val = (int *)G_realloc(val, alloc * sizeof(int));
	}

	switch (type) {
	case (DB_C_TYPE_INT):
	    val[count] = db_get_value_int(value);
	    break;
	case (DB_C_TYPE_STRING):
	    sval = db_get_value_string(value);
	    val[count] = atoi(sval);
	    break;
	case (DB_C_TYPE_DOUBLE):
	    val[count] = (int)db_get_value_double(value);
	    break;
	default:
	    return (-1);
	}
	count++;
    }

    db_close_cursor(&cursor);
    db_free_string(&stmt);

    qsort((void *)val, count, sizeof(int), cmp);

    *pval = val;

    return (count);
}

/*!
  \brief Select one (first) value from table/column for key/id

  \param driver DB driver
  \param tab table name
  \param key key column name
  \param id identifier in key column
  \param col name of column to select the value from
  \param[out] val dbValue to store within

  \return number of selected values
  \return -1 on error
 */
int db_select_value(dbDriver * driver, const char *tab, const char *key,
		    int id, const char *col, dbValue * val)
{
    int more, count;
    char *buf = NULL;
    dbString stmt;
    dbCursor cursor;
    dbColumn *column;
    dbValue *value;
    dbTable *table;

    if (key == NULL || strlen(key) == 0) {
	G_warning(_("Missing key column name"));
	return -1;
    }

    if (col == NULL || strlen(col) == 0) {
	G_warning(_("Missing column name"));
	return -1;
    }

    G_zero(val, sizeof(dbValue));
    G_asprintf(&buf, "SELECT %s FROM %s WHERE %s = %d", col, tab, key, id);
    db_init_string(&stmt);
    db_set_string(&stmt, buf);
    G_free(buf);

    if (db_open_select_cursor(driver, &stmt, &cursor, DB_SEQUENTIAL) != DB_OK)
	return (-1);

    table = db_get_cursor_table(&cursor);
    column = db_get_table_column(table, 0);	/* first column */
    value = db_get_column_value(column);

    /* fetch the data */
    count = 0;
    while (1) {
	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
	    return (-1);

	if (!more)
	    break;
	if (count == 0)
	    db_copy_value(val, value);
	count++;
    }
    db_close_cursor(&cursor);
    db_free_string(&stmt);

    return (count);
}

/*!
  \brief Select pairs key/value to array, values are sorted by key (must be integer)

  \param driver DB driver
  \param tab table name
  \param key key column name
  \param col value column name
  \param[out] cvarr dbCatValArray to store within

  \return number of selected values
  \return -1 on error
 */
int db_select_CatValArray(dbDriver * driver, const char *tab, const char *key,
			  const char *col, const char *where,
			  dbCatValArray * cvarr)
{
    int i, type, more, nrows, ncols;
    char *buf = NULL;
    dbString stmt;
    dbCursor cursor;
    dbColumn *column;
    dbValue *value;
    dbTable *table;

    G_debug(3, "db_select_CatValArray ()");

    if (key == NULL || strlen(key) == 0) {
	G_warning(_("Missing key column name"));
	return -1;
    }

    if (col == NULL || strlen(col) == 0) {
	G_warning(_("Missing column name"));
	return -1;
    }
    db_init_string(&stmt);

    if (strcmp(key, col) == 0) {
	ncols = 1;
	G_asprintf(&buf, "SELECT %s FROM %s", key, tab);
    }
    else {
	ncols = 2;
	G_asprintf(&buf, "SELECT %s, %s FROM %s", key, col, tab);
    }
    db_set_string(&stmt, buf);
    G_free(buf);

    if (where != NULL && strlen(where) > 0) {
	db_append_string(&stmt, " WHERE ");
	db_append_string(&stmt, where);
    }

    G_debug(3, "  SQL: %s", db_get_string(&stmt));

    if (db_open_select_cursor(driver, &stmt, &cursor, DB_SEQUENTIAL) != DB_OK)
	return (-1);

    nrows = db_get_num_rows(&cursor);
    G_debug(3, "  %d rows selected", nrows);
    if (nrows < 0) {
	G_warning(_("Unable select records from table <%s>"), tab);
	db_close_cursor(&cursor);
	db_free_string(&stmt);
	return -1;
    }

    db_CatValArray_alloc(cvarr, nrows);

    table = db_get_cursor_table(&cursor);

    /* Check if key column is integer */
    column = db_get_table_column(table, 0);
    type = db_sqltype_to_Ctype(db_get_column_sqltype(column));
    G_debug(3, "  key type = %d", type);

    if (type != DB_C_TYPE_INT) {
	G_warning(_("Key column type is not integer"));
	db_close_cursor(&cursor);
	db_free_string(&stmt);
	return -1;
    }

    if (ncols == 2) {
	column = db_get_table_column(table, 1);
	type = db_sqltype_to_Ctype(db_get_column_sqltype(column));
	G_debug(3, "  col type = %d", type);

	/*
	  if ( type != DB_C_TYPE_INT && type != DB_C_TYPE_DOUBLE ) {
	  G_fatal_error ( "Column type not supported by db_select_to_array()" );
	  }
	*/
    }
    cvarr->ctype = type;

    /* fetch the data */
    for (i = 0; i < nrows; i++) {
	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
	    return (-1);

	column = db_get_table_column(table, 0);	/* first column */
	value = db_get_column_value(column);
	cvarr->value[i].cat = db_get_value_int(value);

	if (ncols == 2) {
	    column = db_get_table_column(table, 1);
	    value = db_get_column_value(column);
	}
	cvarr->value[i].isNull = value->isNull;
	switch (type) {
	case (DB_C_TYPE_INT):
	    if (value->isNull)
		cvarr->value[i].val.i = 0;
	    else
		cvarr->value[i].val.i = db_get_value_int(value);
	    break;

	case (DB_C_TYPE_DOUBLE):
	    if (value->isNull)
		cvarr->value[i].val.d = 0.0;
	    else
		cvarr->value[i].val.d = db_get_value_double(value);
	    break;

	case (DB_C_TYPE_STRING):
	    cvarr->value[i].val.s = (dbString *) malloc(sizeof(dbString));
	    db_init_string(cvarr->value[i].val.s);

	    if (!(value->isNull))
		db_set_string(cvarr->value[i].val.s,
			      db_get_value_string(value));
	    break;

	case (DB_C_TYPE_DATETIME):
	    cvarr->value[i].val.t =
		(dbDateTime *) calloc(1, sizeof(dbDateTime));

	    if (!(value->isNull))
		memcpy(cvarr->value[i].val.t, &(value->t),
		       sizeof(dbDateTime));
	    break;

	default:
	    return (-1);
	}
    }
    cvarr->n_values = nrows;

    db_close_cursor(&cursor);
    db_free_string(&stmt);

    db_CatValArray_sort(cvarr);

    return nrows;
}

/*!
  \brief Sort key/value array by key
  \param[in,out] arr dbCatValArray (key/value array)
*/
void db_CatValArray_sort(dbCatValArray * arr)
{
    qsort((void *)arr->value, arr->n_values, sizeof(dbCatVal), cmpcat);
}

/*!
  \brief Sort key/value array by value
  
  \param[in,out] arr dbCatValArray (key/value array)
  
  \return DB_OK on success
  \return DB_FAILED on error
 */
int db_CatValArray_sort_by_value(dbCatValArray * arr)
{
    switch (arr->ctype) {
    case (DB_C_TYPE_INT):
	qsort((void *)arr->value, arr->n_values, sizeof(dbCatVal),
	      cmpvalueint);
	break;
    case (DB_C_TYPE_DOUBLE):
	qsort((void *)arr->value, arr->n_values, sizeof(dbCatVal),
	      cmpvaluedouble);
	break;
    case (DB_C_TYPE_STRING):
	qsort((void *)arr->value, arr->n_values, sizeof(dbCatVal),
	      cmpvaluestring);
	break;
    case (DB_C_TYPE_DATETIME):	/* is cmpvaluestring right here ? */
	qsort((void *)arr->value, arr->n_values, sizeof(dbCatVal),
	      cmpvaluestring);
	break;
    default:
	return (DB_FAILED);
    }

    return (DB_OK);
}

/*!
  \brief Find value by key

  \param arr dbCatValArray (key/value array)
  \param key key value
  \param[out] cv dbCatVal structure (key/value) to store within

  \return DB_OK on success
  \return DB_FAILED on error
 */
int db_CatValArray_get_value(dbCatValArray * arr, int key, dbCatVal ** cv)
{
    dbCatVal *catval;

    catval =
	bsearch((void *)&key, arr->value, arr->n_values, sizeof(dbCatVal),
		cmpcat);
    if (catval == NULL) {
	return DB_FAILED;
    }

    *cv = catval;

    return DB_OK;
}

/*!
  \brief Find value (integer) by key

  \param arr dbCatValArray (key/value array)
  \param key key value
  \param[out] val found value (integer)

  \return DB_OK on success
  \return DB_FAILED on error
 */
int db_CatValArray_get_value_int(dbCatValArray * arr, int key, int *val)
{
    dbCatVal *catval;

    catval =
	bsearch((void *)&key, arr->value, arr->n_values, sizeof(dbCatVal),
		cmpcat);
    if (catval == NULL) {
	return DB_FAILED;
    }

    *val = catval->val.i;

    return DB_OK;
}

/*!
  \brief Find value (double) by key
  
  \param arr dbCatValArray (key/value array)
  \param key key value
  \param[out] val found value (double)

  \return DB_OK on success
  \return DB_FAILED on error
*/
int db_CatValArray_get_value_double(dbCatValArray * arr, int key, double *val)
{
    dbCatVal *catval;

    G_debug(3, "db_CatValArray_get_value_double(), key = %d", key);

    catval =
	bsearch((void *)&key, arr->value, arr->n_values, sizeof(dbCatVal),
		cmpcatkey);
    if (catval == NULL) {
	return DB_FAILED;
    }

    *val = catval->val.d;

    return DB_OK;
}
