
/*****************************************************************************
*
* MODULE:       DBF driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* PURPOSE:      Simple driver for reading and writing dbf files     
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
* DBF API:      http://shapelib.maptools.org/dbf_api.html
*
* DBFFieldType: FTString, FTInteger, FTDouble, FTLogical, FTInvalid
*                  0          1          2         4         5
*                DBF_CHAR   DBF_INT   DBF_DOUBLE  
*                  1          2          3
*****************************************************************************/
#ifdef __MINGW32__
#  include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grass/dbmi.h>
#include <grass/shapefil.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

/* add table to database */
int add_table(char *table, char *name)
{
    G_debug(2, "add_table(): table = %s name = %s", table, name);

    if (db.atables == db.ntables) {
	db.atables += 15;
	db.tables =
	    (TABLE *) G_realloc(db.tables, db.atables * sizeof(TABLE));
    }

    strcpy(db.tables[db.ntables].name, table);

#ifdef __MINGW32__
    sprintf(db.tables[db.ntables].file, "%s\\%s", db.name, name);
#else
    sprintf(db.tables[db.ntables].file, "%s/%s", db.name, name);
#endif

    db.tables[db.ntables].alive = TRUE;
    db.tables[db.ntables].described = FALSE;
    db.tables[db.ntables].loaded = FALSE;
    db.tables[db.ntables].updated = FALSE;
    db.tables[db.ntables].cols = NULL;
    db.tables[db.ntables].rows = NULL;
    db.tables[db.ntables].acols = 0;
    db.tables[db.ntables].ncols = 0;
    db.tables[db.ntables].arows = 0;
    db.tables[db.ntables].nrows = 0;

    db.ntables++;

    return DB_OK;
}


/* returns table index or -1 */
int find_table(char *table)
{
    int i;

    G_debug(2, "find_table(): table = %s", table);

    for (i = 0; i < db.ntables; i++) {
	G_debug(2, "  ? %s", db.tables[i].name);
	if (G_strcasecmp(db.tables[i].name, table) == 0)
	    return (i);
    }

    return (-1);
}

int load_table_head(int t)
{
    int i, ncol, dtype, type, width, decimals;
    DBFHandle dbf;
    char fname[20];

    G_debug(2, "load_table_head(): tab = %d, %s", t, db.tables[t].file);

    if (db.tables[t].described == TRUE)	/*already described */
	return DB_OK;

    if (access(db.tables[t].file, R_OK) == 0)
	db.tables[t].read = TRUE;
    else
	db.tables[t].read = FALSE;

    if (access(db.tables[t].file, W_OK) == 0)
	db.tables[t].write = TRUE;
    else
	db.tables[t].write = FALSE;

    /* load */
    dbf = DBFOpen(db.tables[t].file, "r");
    if (dbf == NULL) {
	db_d_append_error(_("Unable to open DBF file."));
	return DB_FAILED;
    }

    ncol = DBFGetFieldCount(dbf);
    G_debug(2, "  ncols = %d", ncol);

    for (i = 0; i < ncol; i++) {
	dtype = DBFGetFieldInfo(dbf, i, fname, &width, &decimals);
	G_debug(2, "  DBFFieldType %d", dtype);

	switch (dtype) {
	case FTString:
	    type = DBF_CHAR;
	    break;
	case FTInteger:
	    type = DBF_INT;
	    break;
	case FTDouble:
	    type = DBF_DOUBLE;
	    break;
	case FTInvalid:
	    G_warning("invalid/unsupported DBFFieldType");
	    break;
	default:
	    G_warning("unknown DBFFieldType");
	    break;
	}

	add_column(t, type, fname, width, decimals);
    }

    DBFClose(dbf);
    db.tables[t].described = TRUE;

    return DB_OK;
}

int load_table(int t)
{
    int i, j, ncols, nrows, dbfcol;
    DBFHandle dbf;
    char *buf;
    ROW *rows;
    VALUE *val;

    G_debug(2, "load_table(): tab = %d", t);

    if (db.tables[t].loaded == TRUE)	/*already loaded */
	return DB_OK;

    dbf = DBFOpen(db.tables[t].file, "r");
    if (dbf == NULL) {
	db_d_append_error(_("Unable to open DBF file."));
	return DB_FAILED;
    }

    ncols = db.tables[t].ncols;
    nrows = DBFGetRecordCount(dbf);
    rows = db.tables[t].rows;
    rows = (ROW *) G_malloc(nrows * sizeof(ROW));
    db.tables[t].arows = nrows;

    G_debug(2, "  ncols = %d nrows = %d", ncols, nrows);

    for (i = 0; i < nrows; i++) {
	rows[i].alive = TRUE;
	rows[i].values = (VALUE *) G_calloc(ncols, sizeof(VALUE));

	for (j = 0; j < ncols; j++) {
	    val = &(rows[i].values[j]);

	    dbfcol = j;

	    val->is_null = DBFIsAttributeNULL(dbf, i, dbfcol);
	    if (!(val->is_null)) {
		switch (db.tables[t].cols[j].type) {
		case DBF_INT:
		    val->i = DBFReadIntegerAttribute(dbf, i, dbfcol);
		    break;
		case DBF_CHAR:
		    buf = (char *)DBFReadStringAttribute(dbf, i, dbfcol);
		    save_string(val, buf);
		    break;
		case DBF_DOUBLE:
		    val->d = DBFReadDoubleAttribute(dbf, i, dbfcol);
		    break;
		}
	    }
	}
    }

    DBFClose(dbf);

    db.tables[t].rows = rows;
    db.tables[t].nrows = nrows;
    db.tables[t].loaded = TRUE;

    return DB_OK;
}

int save_table(int t)
{
    int i, j, ncols, nrows, ret, field, rec;
    char name[2000], fname[20], element[100];
    DBFHandle dbf;
    ROW *rows;
    VALUE *val;
    int dbftype, width, decimals;

    G_debug(2, "save_table %d", t);

    /* Note: because if driver is killed during the time the table is written, the process
     *        is not completed and DATA ARE LOST. To minimize this, data are first written
     *        to temporary file and then this file is renamed to 'database/table.dbf'.
     *        Hopefully both file are on the same disk/partition */

    if (!(db.tables[t].alive) || !(db.tables[t].updated))
	return DB_OK;

    /* Construct our temp name because shapelib doesn't like '.' in name */
    G_temp_element(element);
    sprintf(fname, "%d.dbf", getpid());
    G_file_name(name, element, fname, G_mapset());
    G_debug(2, "Write table to tempfile: '%s'", name);

    dbf = DBFCreate(name);
    if (dbf == NULL)
	return DB_FAILED;

    ncols = db.tables[t].ncols;
    rows = db.tables[t].rows;
    nrows = db.tables[t].nrows;

    for (i = 0; i < ncols; i++) {
	switch (db.tables[t].cols[i].type) {
	case DBF_INT:
	    dbftype = FTInteger;
	    break;
	case DBF_CHAR:
	    dbftype = FTString;
	    break;
	case DBF_DOUBLE:
	    dbftype = FTDouble;
	    break;
	default:
	    G_warning("invalid/unsupported DBFFieldType");
	    break;
	}

	width = db.tables[t].cols[i].width;
	decimals = db.tables[t].cols[i].decimals;
	DBFAddField(dbf, db.tables[t].cols[i].name, dbftype, width, decimals);

    }

    G_debug(2, "Write %d rows", nrows);
    rec = 0;
    for (i = 0; i < nrows; i++) {
	if (rows[i].alive == FALSE)
	    continue;

	for (j = 0; j < ncols; j++) {
	    field = j;

	    val = &(rows[i].values[j]);
	    if (val->is_null) {
		DBFWriteNULLAttribute(dbf, rec, field);
	    }
	    else {
		switch (db.tables[t].cols[j].type) {
		case DBF_INT:
		    ret = DBFWriteIntegerAttribute(dbf, rec, field, val->i);
		    break;
		case DBF_CHAR:
		    if (val->c != NULL)
			ret =
			    DBFWriteStringAttribute(dbf, rec, field, val->c);
		    else
			ret = DBFWriteStringAttribute(dbf, rec, field, "");
		    break;
		case DBF_DOUBLE:
		    ret = DBFWriteDoubleAttribute(dbf, rec, field, val->d);
		    break;
		}
	    }
	}
	rec++;
    }
    G_debug(2, "Written %d records", rec);

    DBFClose(dbf);

    /* Copy */
    if (G_rename_file(name, db.tables[t].file)) {
	db_d_append_error(_("Unable to move '%s' to '%s'."),
			    name, db.tables[t].file);
	return DB_FAILED;
    };

    return DB_OK;
}

int free_table(int tab)
{
    int i, j;

    for (i = 0; i < db.tables[tab].nrows; i++) {
	for (j = 0; j < db.tables[tab].ncols; j++) {
	    if (db.tables[tab].cols[j].type == DBF_CHAR &&
		db.tables[tab].rows[i].values[j].c != NULL) {
		G_free(db.tables[tab].rows[i].values[j].c);
	    }
	}
	G_free(db.tables[tab].rows[i].values);
    }

    G_free(db.tables[tab].rows);

    return DB_OK;
}
