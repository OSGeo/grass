/****************************************************************************
 *
 * MODULE:       db.select
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Huidae Cho <grass4u gmail.com>, Glynn Clements <glynn gclements.plus.com>, Jachym Cepicky <jachym les-ejk.cz>, Markus Neteler <neteler itc.it>, Stephan Holl
 * PURPOSE:      process one sql select statement
 * COPYRIGHT:    (C) 2002-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/codes.h>
#include <grass/glocale.h>
#include "local_proto.h"


struct {
	char *driver, *database, *table, *sql, *fs, *vs, *nv, *input;
	int c,d,h, test_only;
} parms;


/* function prototypes */
static void parse_command_line (int, char **);
static int sel (dbDriver *, dbString *);
static int get_stmt (FILE *, dbString *);
static int stmt_is_empty (dbString *);


int
main (int argc, char **argv)
{
    dbString stmt;
    dbDriver *driver;
    dbHandle handle;
    int stat;
    FILE *fd;

    parse_command_line (argc, argv);

    if (parms.input)
    {
	fd = fopen (parms.input, "r");
	if (fd == NULL)
	{
	    perror (parms.input);
	    exit(ERROR);
	}
    }
    else
	fd = stdin;
       
    db_init_string ( &stmt );
    
    driver = db_start_driver(parms.driver);
    if (driver == NULL) {
	G_fatal_error(_("Unable to start driver <%s>"), parms.driver);
    }

    db_init_handle (&handle);
    db_set_handle (&handle, parms.database, NULL);
    if (db_open_database(driver, &handle) != DB_OK)
      	G_fatal_error(_("Unable to open database <%s>"), parms.database);

    if ( parms.sql ) {
	db_set_string ( &stmt, parms.sql );
	stat = sel(driver, &stmt );
    } else if ( parms.table ) {
	db_set_string ( &stmt, "select * from "); 
	db_append_string ( &stmt, parms.table); 
	stat = sel(driver, &stmt);
    } else { /* read stdin */
	stat = OK;
	while(stat == OK && get_stmt (fd, &stmt))
	{
	    if(!stmt_is_empty(&stmt))
		stat = sel(driver, &stmt);
	}
    }

    db_close_database(driver);
    db_shutdown_driver(driver);

    exit(stat);
}


static int
sel (dbDriver *driver, dbString *stmt)
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

    table = db_get_cursor_table (&cursor);
    ncols = db_get_table_number_of_columns (table);
    if(parms.d)
    {
	for(col = 0; col < ncols; col++)
	{
	    column = db_get_table_column(table, col);
	    print_column_definition(column);
	}

	return OK;
    }

    db_init_string (&value_string);

    /* column names if horizontal output */
    if (parms.h && parms.c)
    {
	for (col = 0; col < ncols; col++)
	{
	    column = db_get_table_column(table, col);
	    if (col) fprintf (stdout, "%s", parms.fs);
	    fprintf (stdout, "%s", db_get_column_name (column));
	}
	fprintf (stdout, "\n");
    }

    /* fetch the data */
    while(1)
    {
	if(db_fetch (&cursor, DB_NEXT, &more) != DB_OK)
	    return ERROR;
	if (!more)
	    break;

	for (col = 0; col < ncols; col++)
	{
	    column = db_get_table_column(table, col);
	    value  = db_get_column_value(column);
	    db_convert_column_value_to_string (column, &value_string);
	    if (parms.c && !parms.h)
		fprintf (stdout, "%s%s", db_get_column_name (column), parms.fs);
	    if (col && parms.h)
		fprintf (stdout, "%s", parms.fs);
	    if(parms.nv && db_test_value_isnull(value))
		fprintf (stdout, "%s", parms.nv);
	    else
		fprintf (stdout, "%s", db_get_string (&value_string));
	    if (!parms.h)
		fprintf (stdout, "\n");
	}
	if (parms.h)
	    fprintf (stdout, "\n");
	else if (parms.vs)
	    fprintf (stdout, "%s\n", parms.vs);
    }

    return OK;
}


static void
parse_command_line (int argc, char **argv)
{
    struct Option *driver, *database, *table, *sql, *fs, *vs, *nv, *input;
    struct Flag *c,*d,*v, *flag_test;
    struct GModule *module;
    char *drv, *db;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]) ;

    table 		= G_define_standard_option(G_OPT_TABLE);

    database 		= G_define_standard_option(G_OPT_DATABASE);
    if ( (db=db_get_default_database_name()) )
        database->answer = db;

    driver 		= G_define_standard_option(G_OPT_DRIVER);
    driver->options     = db_list_drivers();
    if ( (drv=db_get_default_driver_name()) )
        driver->answer = drv;

    sql 		= G_define_option();
    sql->key 	        = "sql";
    sql->type 	        = TYPE_STRING;
    sql->required 	= NO;         
    sql->label          = _("SQL select statement");
    sql->description    = _("For example: 'select * from rybniky where kapri = 'hodne'");

    fs 			= G_define_standard_option(G_OPT_F_SEP);
    fs->description 	= _("Output field separator");

    vs 			= G_define_standard_option(G_OPT_F_SEP);
    vs->key 		= "vs";
    vs->description 	= _("Output vertical record separator");
    vs->answer          = NULL;

    nv 			= G_define_option();
    nv->key 		= "nv";
    nv->type 		= TYPE_STRING;
    nv->required 	= NO;
    nv->description 	= _("Null value indicator");

    input 		= G_define_standard_option(G_OPT_F_INPUT);
    input->required 	= NO;
    input->description 	= _("Name of file with sql statement");

    c			= G_define_flag();
    c->key		= 'c';
    c->description	= _("Do not include column names in output");

    d			= G_define_flag();
    d->key		= 'd';
    d->description	= _("Describe query only (don't run it)");

    v			= G_define_flag();
    v->key		= 'v';
    v->description	= _("Vertical output (instead of horizontal)");

    flag_test			= G_define_flag();
    flag_test->key		= 't';
    flag_test->description	= _("Only test query, do not execute");

    /* Set description */
    module              = G_define_module();
    module->keywords = _("database, SQL");
    module->description = _("Selects data from table.");

    if(G_parser(argc, argv))
        exit(EXIT_SUCCESS);

    parms.driver	= driver->answer;
    parms.database	= database->answer;
    parms.table 	= table->answer;
    parms.sql  	        = sql->answer;
    parms.fs		= fs->answer;
    parms.vs		= vs->answer;
    parms.nv		= nv->answer;
    parms.input		= input->answer;
    if ( !c->answer )  parms.c = 1; else  parms.c = 0;
    parms.d		= d->answer;
    if ( !v->answer )  parms.h = 1; else  parms.h = 0;
    parms.test_only     = flag_test->answer;

    if (!parms.fs) parms.fs = "";
    if (parms.input && *parms.input == 0)
    {
	G_usage();
	exit(EXIT_FAILURE);
    }
}


static int
get_stmt (FILE *fd, dbString *stmt)
{
    char buf[1024];
    int n;
    static int first = 1;

    db_zero_string (stmt);

    /* this is until get_stmt is smart enough to handle multiple stmts */
    if (!first)
	return 0;
    first = 0;

    while ( ( n = fread (buf, 1, sizeof(buf)-1, fd)) > 0)
    {
	buf[n] = 0;
	db_append_string (stmt, buf);
    }

    return 1;
}


static int
stmt_is_empty (dbString *stmt)
{
    char dummy[2];

    return (sscanf (db_get_string(stmt), "%1s", dummy) != 1);
}
