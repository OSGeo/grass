
/****************************************************************************
 *
 * MODULE:       db.select
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Huidae Cho <grass4u gmail.com>,
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>,
 *               Markus Neteler <neteler itc.it>
 *               Stephan Holl
 * PURPOSE:      Process one sql select statement
 * COPYRIGHT:    (C) 2002-2010, 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "local_proto.h"

struct {
    char *driver, *database, *table, *sql, fs, vs, *nv, *input, *output;
    int c, d, h, test_only;
} parms;


/* function prototypes */
static void parse_command_line(int, char **);
static int sel(dbDriver *, dbString *);
static int get_stmt(FILE *, dbString *);
static int stmt_is_empty(dbString *);

int main(int argc, char **argv)
{
    dbString stmt;
    dbDriver *driver;
    dbHandle handle;
    int stat;
    FILE *fd;

    parse_command_line(argc, argv);

    /* read from file or stdin ? */
    if (parms.input && strcmp(parms.input, "-") != 0) {
	fd = fopen(parms.input, "r");
	if (fd == NULL) {
	    G_fatal_error(_("Unable to open file <%s>: %s"),
                          parms.input, strerror(errno));
	}
    }
    else
	fd = stdin;

    /* open DB connection */
    db_init_string(&stmt);

    driver = db_start_driver(parms.driver);
    if (driver == NULL) {
	G_fatal_error(_("Unable to start driver <%s>"), parms.driver);
    }

    db_init_handle(&handle);
    db_set_handle(&handle, parms.database, NULL);
    if (db_open_database(driver, &handle) != DB_OK)
	G_fatal_error(_("Unable to open database <%s>"), parms.database);

    /* check for sql, table, and input */
    if (parms.sql) {
        /* parms.sql */
        db_set_string(&stmt, parms.sql);
        stat = sel(driver, &stmt);
    }
    else if (parms.table) {
        /* parms.table */
	db_set_string(&stmt, "SELECT * FROM ");
	db_append_string(&stmt, parms.table);
	stat = sel(driver, &stmt);
    }
    else { /* -> parms.input */
        stat = OK;
        while (stat == OK && get_stmt(fd, &stmt)) {
            if (!stmt_is_empty(&stmt))
                stat = sel(driver, &stmt);
        }
    }

    if(parms.test_only)
	G_verbose_message(_("Test %s."), stat ? _("failed") : _("succeeded"));

    db_close_database(driver);
    db_shutdown_driver(driver);

    exit(stat == OK ? EXIT_SUCCESS : EXIT_FAILURE);
}


int sel(dbDriver * driver, dbString * stmt)
{
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;
    dbString value_string;
    int col, ncols;
    int more;

    if (db_open_select_cursor(driver, stmt, &cursor, DB_SEQUENTIAL) != DB_OK)
	return ERROR;
    if (parms.test_only)
	return OK;

    table = db_get_cursor_table(&cursor);
    ncols = db_get_table_number_of_columns(table);
    if (parms.d) {
	for (col = 0; col < ncols; col++) {
	    column = db_get_table_column(table, col);
	    print_column_definition(column);
	}

	return OK;
    }

    if (parms.output && strcmp(parms.output, "-") != 0) { 
	if (NULL == freopen(parms.output, "w", stdout)) { 
	    G_fatal_error(_("Unable to open file <%s> for writing"), parms.output); 
	} 
    } 
    
    db_init_string(&value_string);

    /* column names if horizontal output */
    if (parms.h && parms.c) {
	for (col = 0; col < ncols; col++) {
	    column = db_get_table_column(table, col);
	    if (col)
		fprintf(stdout, "%s", &parms.fs);
	    fprintf(stdout, "%s", db_get_column_name(column));
	}
	fprintf(stdout, "\n");
    }

    /* fetch the data */
    while (TRUE) {
	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
	    return ERROR;
	if (!more)
	    break;

	for (col = 0; col < ncols; col++) {
	    column = db_get_table_column(table, col);
	    value = db_get_column_value(column);
	    db_convert_column_value_to_string(column, &value_string);
	    if (parms.c && !parms.h)
		fprintf(stdout, "%s%s", db_get_column_name(column), &parms.fs);
	    if (col && parms.h)
		fprintf(stdout, "%s", &parms.fs);
	    if (parms.nv && db_test_value_isnull(value))
		fprintf(stdout, "%s", parms.nv);
	    else
		fprintf(stdout, "%s", db_get_string(&value_string));
	    if (!parms.h)
		fprintf(stdout, "\n");
	}
	if (parms.h)
	    fprintf(stdout, "\n");
	else if (parms.vs)
	    fprintf(stdout, "%s\n", &parms.vs);
    }

    return OK;
}


void parse_command_line(int argc, char **argv)
{
    struct Option *driver, *database, *table, *sql,
      *fs, *vs, *nv, *input, *output;
    struct Flag *c, *d, *v, *flag_test;
    struct GModule *module;
    const char *drv, *db;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    sql = G_define_standard_option(G_OPT_DB_SQL);
    sql->guisection = _("Query");

    input = G_define_standard_option(G_OPT_F_INPUT);
    input->required = NO;
    input->label = _("Name of file containing SQL select statement(s)");
    input->description = _("'-' for standard input");
    input->guisection = _("Query");
    
    table = G_define_standard_option(G_OPT_DB_TABLE);
    table->description = _("Name of table to query");
    table->guisection = _("Query");

    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = db_list_drivers();
    if ((drv = db_get_default_driver_name()))
	driver->answer = (char *) drv;
    driver->guisection = _("Connection");

    database = G_define_standard_option(G_OPT_DB_DATABASE);
    if ((db = db_get_default_database_name()))
	database->answer = (char *) db;
    database->guisection = _("Connection");
    
    fs = G_define_standard_option(G_OPT_F_SEP);
    fs->guisection = _("Format");

    vs = G_define_standard_option(G_OPT_F_SEP);
    vs->key = "vs";
    vs->label = _("Output vertical record separator");
    vs->answer = NULL;
    vs->guisection = _("Format");

    nv = G_define_option();
    nv->key = "nv";
    nv->type = TYPE_STRING;
    nv->required = NO;
    nv->description = _("Null value indicator");
    nv->guisection = _("Format");

    output = G_define_standard_option(G_OPT_F_OUTPUT); 
    output->required = NO; 
    output->description = 
	_("Name for output file (if omitted or \"-\" output to stdout)"); 
    
    c = G_define_flag();
    c->key = 'c';
    c->description = _("Do not include column names in output");
    c->guisection = _("Format");

    d = G_define_flag();
    d->key = 'd';
    d->description = _("Describe query only (don't run it)");
    d->guisection = _("Query");

    v = G_define_flag();
    v->key = 'v';
    v->description = _("Vertical output (instead of horizontal)");
    v->guisection = _("Format");

    flag_test = G_define_flag();
    flag_test->key = 't';
    flag_test->description = _("Only test query, do not execute");
    flag_test->guisection = _("Query");

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("SQL"));
    module->label = _("Selects data from attribute table.");
    module->description = _("Performs SQL query statement(s).");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    parms.driver = driver->answer;
    parms.database = database->answer;
    parms.table = table->answer;
    parms.sql = sql->answer;
    parms.fs = G_option_to_separator(fs);
    if (vs->answer)
	parms.vs = G_option_to_separator(vs);
    else
	parms.vs = vs->answer;
    parms.nv = nv->answer;
    parms.input = input->answer;
    parms.output = output->answer;

    if (!c->answer)
	parms.c = TRUE;
    else
	parms.c = FALSE;
    parms.d = d->answer;
    if (!v->answer)
	parms.h = TRUE;
    else
	parms.h = FALSE;

    parms.test_only = flag_test->answer;
    
    if (parms.input && *parms.input == 0) {
	G_usage();
	exit(EXIT_FAILURE);
    }
    
    if (!parms.input && !parms.sql && !parms.table)
        G_fatal_error(_("You must provide one of these options: <%s>, <%s>, or <%s>"),
                      sql->key, input->key, table->key);
}

int get_stmt(FILE * fd, dbString * stmt)
{
    char buf[DB_SQL_MAX], buf2[DB_SQL_MAX];
    size_t len;
    
    db_zero_string(stmt);
    
    if (G_getl2(buf, sizeof(buf), fd) == 0)
        return 0;
        
    strcpy(buf2, buf);
    G_chop(buf2);
    len = strlen(buf2);
        
    if (buf2[len - 1] == ';') { /* end of statement */
        buf2[len - 1] = 0;      /* truncate ';' */
    }
    
    db_set_string(stmt, buf);
    
    return 1;
}


int stmt_is_empty(dbString * stmt)
{
    char dummy[2];

    return (sscanf(db_get_string(stmt), "%1s", dummy) != 1);
}
