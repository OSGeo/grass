#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <tcl.h>
#include <tk.h>
#include <locale.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/form.h>

/* Structure to store column names and values */
typedef struct
{
    char *name;
    int ctype;
    char *value;
} COLUMN;

static char *Drvname, *Dbname, *Tblname, *Key;

static COLUMN *Columns = NULL;
static int allocatedRows = 0;			/* allocated space */
static int nRows = 0;

/* Start new sql update */
int reset_values(ClientData cdata, Tcl_Interp * interp, int argc, char *argv[])
{
    nRows = 0;
    Drvname = NULL;
    Dbname = NULL;
    Tblname = NULL;
    Key = NULL;

    return TCL_OK;
}

int set_value(ClientData cdata, Tcl_Interp * interp, int argc, char *argv[])
{
    G_debug(2, "set_value(): %s %s", argv[1], argv[2]);

    if (strcmp(argv[1], F_DRIVER_FNAME) == 0) {
	Drvname = G_store(argv[2]);
    }
    else if (strcmp(argv[1], F_DATABASE_FNAME) == 0) {
	Dbname = G_store(argv[2]);
    }
    else if (strcmp(argv[1], F_TABLE_FNAME) == 0) {
	Tblname = G_store(argv[2]);
    }
    else if (strcmp(argv[1], F_KEY_FNAME) == 0) {
	Key = G_store(argv[2]);
    }
    else {
	if (nRows == allocatedRows) {
	    allocatedRows += 100;
	    Columns = (COLUMN *) G_realloc(Columns, (allocatedRows) * sizeof(COLUMN));
	}
	Columns[nRows].name = G_store(argv[1]);
	Columns[nRows].value = G_store(argv[2]);
	nRows++;
    }

    return TCL_OK;
}

/* Update table, use the data previously stored by set_value() */
int submit(ClientData cdata, Tcl_Interp * interp, int argc, char *argv[])
{
    int i, first, ncols, found, col, sqltype, keyval = 0, ret;
    char buf[2001];
    dbString sql, table_name, strval;
    dbDriver *driver;
    dbHandle handle;
    dbTable *table;
    dbColumn *column;

    G_debug(2, "submit()");

    db_init_string(&sql);
    db_init_string(&table_name);
    db_init_string(&strval);

    /* Check if all internal values are set */
    if (Drvname == NULL || Dbname == NULL || Tblname == NULL || Key == NULL) {
	G_warning("db connection was not set by form\n");
	sprintf(buf, "set submit_msg \"db connection was not set by form.\"");
	Tcl_Eval(interp, buf);
	Tcl_Eval(interp, "set submit_result 0");
	return TCL_OK;
    }

    /* Get column types */
    G_debug(2, "Open driver");
    driver = db_start_driver(Drvname);
    if (driver == NULL) {
	G_warning("Cannot open driver\n");
	sprintf(buf, "set submit_msg \"Cannot open driver '%s'\"", Drvname);
	Tcl_Eval(interp, buf);
	Tcl_Eval(interp, "set submit_result 0");
	return TCL_OK;
    }
    G_debug(2, "Driver opened");

    db_init_handle(&handle);
    db_set_handle(&handle, Dbname, NULL);
    G_debug(2, "Open database");
    if (db_open_database(driver, &handle) != DB_OK) {
	G_warning("Cannot open database\n");
	db_shutdown_driver(driver);
	sprintf(buf,
		"set submit_msg \"Cannot open database '%s' by driver '%s'\"",
		Dbname, Drvname);
	Tcl_Eval(interp, buf);
	Tcl_Eval(interp, "set submit_result 0");
	return TCL_OK;
    }
    G_debug(2, "Database opened");

    db_set_string(&table_name, Tblname);
    if (db_describe_table(driver, &table_name, &table) != DB_OK) {
	G_warning("Cannot describe table\n");
	db_shutdown_driver(driver);
	db_close_database(driver);
	sprintf(buf, "set submit_msg \"Cannot describe table '%s'\"", Tblname);
	Tcl_Eval(interp, buf);
	Tcl_Eval(interp, "set submit_result 0");
	return TCL_OK;
    }
    ncols = db_get_table_number_of_columns(table);

    /* For each column get ctype */
    for (i = 0; i < nRows; i++) {
	found = 0;
	for (col = 0; col < ncols; col++) {
	    /* get keyval */
	    if (G_strcasecmp(Columns[i].name, Key) == 0) {
		keyval = atoi(Columns[i].value);
	    }
	    column = db_get_table_column(table, col);
	    if (G_strcasecmp(db_get_column_name(column), Columns[i].name) == 0) {
		sqltype = db_get_column_sqltype(column);
		Columns[i].ctype = db_sqltype_to_Ctype(sqltype);
		found = 1;
		break;
	    }
	}
	if (!found && (G_strcasecmp(Columns[i].name, F_ENCODING) != 0)) {
	    G_warning("Cannot find column type");
	    db_close_database(driver);
	    db_shutdown_driver(driver);
	    sprintf(buf, "set submit_msg \"Cannot find column type\"");
	    Tcl_Eval(interp, buf);
	    Tcl_Eval(interp, "set submit_result 0");
	    return TCL_OK;
	}
    }

    /* Construct update statement */
    sprintf(buf, "update %s set ", Tblname);
    db_set_string(&sql, buf);

    first = 1;
    for (i = 0; i < nRows; i++) {
	G_debug(3, "Index = %d of %d Name = %s, Key = %s", i, nRows, Columns[i].name,Key);
	if (G_strcasecmp(Columns[i].name, Key) == 0)
	    continue;

	if (G_strcasecmp(Columns[i].name, F_ENCODING) == 0) {

	    G_debug(3, "GRASS_DB_ENCODING env-var is '%s', col val is '%s'", G__getenv("GRASS_DB_ENCODING"),
		    Columns[i].value);

	    if ( (strlen(Columns[i].value) == 0) || G_strcasecmp(Columns[i].value, G__getenv("GRASS_DB_ENCODING")) == 0)
		continue;
	    else {
		G_setenv("GRASS_DB_ENCODING", Columns[i].value);
		G_debug(3, "Set env var GRASS_DB_ENCODING to '%s'", Columns[i].value);
		if (Tcl_SetSystemEncoding(interp, Columns[i].value) == TCL_ERROR) {
		    G_warning("Could not set Tcl system encoding to '%s' (%s)",
			    Columns[i].value,interp->result);
		}
	    }
	    continue;
	}

	if (!first) {
	    db_append_string(&sql, ", ");
	}
	if (strlen(Columns[i].value) == 0) {
	    sprintf(buf, "%s = null", Columns[i].name);
	}
	else {
	    if (Columns[i].ctype == DB_C_TYPE_INT ||
		Columns[i].ctype == DB_C_TYPE_DOUBLE) {
		sprintf(buf, "%s = %s", Columns[i].name, Columns[i].value);
	    }
	    else {
		memset(buf, '\0', strlen(buf));
		ret = Tcl_UtfToExternal(interp,
					Tcl_GetEncoding(interp,
							G__getenv
							("GRASS_DB_ENCODING")),
					Columns[i].value, strlen(Columns[i].value), 0,
					NULL, buf, 2000, NULL, NULL, NULL);

		if (ret != TCL_OK) {
		    G_warning("Could not convert UTF to external.");
		    db_set_string(&strval, Columns[i].value);
		}
		else {
		    db_set_string(&strval, buf);
		}

		db_double_quote_string(&strval);
		sprintf(buf, "%s = '%s'", Columns[i].name, db_get_string(&strval));
	    }
	}
	db_append_string(&sql, buf);
	first = 0;
    }

    sprintf(buf, " where %s = %d", Key, keyval);
    db_append_string(&sql, buf);

    G_debug(2, "SQL: %s", db_get_string(&sql));

    /* Update table */
    ret = db_execute_immediate(driver, &sql);

    db_close_database(driver);
    db_shutdown_driver(driver);

    if (ret != DB_OK) {
	G_warning("Cannot update table");
	Tcl_VarEval(interp, "set submit_msg \"Cannot update table:\n",
		    db_get_error_msg(), "\"", NULL);
	Tcl_Eval(interp, "set submit_result 0");
    }
    else {
	Tcl_Eval(interp, "set submit_msg \"Record successfully updated\"");
	Tcl_Eval(interp, "set submit_result 1");
    }

    return TCL_OK;
}

/* 
 *  Form 
 */
int Tcl_AppInit(Tcl_Interp *interp)
{
	if (Tcl_Init(interp) == TCL_ERROR)
		return TCL_ERROR;

	if (Tk_Init(interp) == TCL_ERROR)
		return TCL_ERROR;

	Tcl_StaticPackage(interp, "Tk", Tk_Init, Tk_SafeInit);

	/*
	 * Call Tcl_CreateCommand for application-specific commands, if
	 * they weren't already created by the init procedures called above.
	 */

	Tcl_CreateCommand(interp, "submit", (Tcl_CmdProc *) submit,
			  (ClientData) NULL,
			  (Tcl_CmdDeleteProc *) NULL);
	Tcl_CreateCommand(interp, "set_value",
			  (Tcl_CmdProc *) set_value,
			  (ClientData) NULL,
			  (Tcl_CmdDeleteProc *) NULL);
	Tcl_CreateCommand(interp, "reset_values",
			  (Tcl_CmdProc *) reset_values,
			  (ClientData) NULL,
			  (Tcl_CmdDeleteProc *) NULL);
	/*
	 * Specify a user-specific startup file to invoke if the application
	 * is run interactively.  Typically the startup file is "~/.apprc"
	 * where "app" is the name of the application.  If this line is deleted
	 * then no user-specific startup file will be run under any conditions.
	 */

	Tcl_SetVar(interp, "tcl_rcFileName", "~/.grassformrc", TCL_GLOBAL_ONLY);
	return TCL_OK;
}

int main(int argc, char *argv[])
{
	G_gisinit("form");
	G_debug(2, "Form: main()");

	Tk_Main(argc, argv, Tcl_AppInit);
	return 0;
}

