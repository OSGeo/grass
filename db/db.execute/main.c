
/****************************************************************************
 *
 * MODULE:       db.execute
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Huidae Cho <grass4u gmail.com>
 *               Glynn Clements <glynn gclements.plus.com>
 *               Hamish Bowman <hamish_b yahoo.com>
 *               Markus Neteler <neteler itc.it>
 *               Stephan Holl
 *               Martin Landa <landa.martin gmail.com>
 * PURPOSE:      process one non-select sql statement
 * COPYRIGHT:    (C) 2002-2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

struct
{
    const char *driver, *database, *schema, *sql, *input;
    int i;
} parms;

/* function prototypes */
static void parse_command_line(int, char **);
static int get_stmt(FILE *, dbString *);
static int stmt_is_empty(dbString *);
static void error_handler(void *);

int main(int argc, char **argv)
{
    dbString stmt;
    dbDriver *driver;
    dbHandle handle;
    int ret;
    FILE *fd;
    int error;

    error = 0;

    parse_command_line(argc, argv);

    /* read from file or stdin ? */
    if (parms.input && strcmp(parms.input, "-") != 0) {
	fd = fopen(parms.input, "r");
	if (fd == NULL) {
	    G_fatal_error(_("Unable to open file <%s>: %s"),
                          parms.input, strerror(errno));
	}
    }
    else {
	fd = stdin;
    }
    
    /* open DB connection */
    db_init_string(&stmt);
    
    driver = db_start_driver(parms.driver);
    if (driver == NULL) {
	G_fatal_error(_("Unable to start driver <%s>"), parms.driver);
    }

    db_init_handle(&handle);
    db_set_handle(&handle, parms.database, parms.schema);
    if (db_open_database(driver, &handle) != DB_OK)
	G_fatal_error(_("Unable to open database <%s>"), parms.database);
    G_add_error_handler(error_handler, driver);
    
    if (parms.sql) {
        /* parms.sql */
        db_set_string(&stmt, parms.sql);
        ret = db_execute_immediate(driver, &stmt);

	if (ret != DB_OK) {
	    if (parms.i) {	/* ignore SQL errors */
		G_warning(_("Error while executing: '%s'"),
			  db_get_string(&stmt));
		error++;
	    }
	    else {
		G_fatal_error(_("Error while executing: '%s'"),
			      db_get_string(&stmt));
	    }
	}
    }
    else { /* parms.input */
        while (get_stmt(fd, &stmt)) {
            if (stmt_is_empty(&stmt))
                continue;
            G_debug(3, "sql: %s", db_get_string(&stmt));
                
            ret = db_execute_immediate(driver, &stmt);
            
            if (ret != DB_OK) {
                if (parms.i) {	/* ignore SQL errors */
                    G_warning(_("Error while executing: '%s'"),
                              db_get_string(&stmt));
                    error++;
                }
                else {
                    G_fatal_error(_("Error while executing: '%s'"),
                                  db_get_string(&stmt));
                }
            }
        }
    }
    
    db_close_database(driver);
    db_shutdown_driver(driver);

    exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}


static void parse_command_line(int argc, char **argv)
{
    struct Option *driver, *database, *schema, *sql, *input;
    struct Flag *i;
    struct GModule *module;
    const char *drv, *db, *schema_name;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("SQL"));
    module->label = _("Executes any SQL statement.");
    module->description = _("For SELECT statements use 'db.select'.");

    sql = G_define_standard_option(G_OPT_DB_SQL);
    sql->label = _("SQL statement");
    sql->description = _("Example: update rybniky set kapri = 'hodne' where kapri = 'malo'");
    sql->guisection = _("SQL");
    
    input = G_define_standard_option(G_OPT_F_INPUT);
    input->required = NO;
    input->label = _("Name of file containing SQL statement(s)");
    input->description = _("'-' for standard input");
    input->guisection = _("SQL");

    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = db_list_drivers();
    driver->guisection = _("Connection");
    if ((drv = db_get_default_driver_name()))
	driver->answer = (char *) drv;
    
    database = G_define_standard_option(G_OPT_DB_DATABASE);
    database->guisection = _("Connection");
    if ((db = db_get_default_database_name()))
	database->answer = (char *) db;

    schema = G_define_standard_option(G_OPT_DB_SCHEMA);
    schema->guisection = _("Connection");
    if ((schema_name = db_get_default_schema_name()))
	schema->answer = (char *) schema_name;

    i = G_define_flag();
    i->key = 'i';
    i->description = _("Ignore SQL errors and continue");
    i->guisection = _("Errors");
    
    if (G_parser(argc, argv))
	exit(EXIT_SUCCESS);

    if (!sql->answer && !input->answer) {
        G_fatal_error(_("You must provide <%s> or <%s> option"),
                      sql->key, input->key);
    }
    
    parms.driver = driver->answer;
    parms.database = database->answer;
    parms.schema = schema->answer;
    parms.sql = sql->answer;
    parms.input = input->answer;
    parms.i = i->answer ? TRUE : FALSE;
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
    
    db_set_string(stmt, buf2);

    return 1;
}

int stmt_is_empty(dbString * stmt)
{
    char dummy[2];

    return (sscanf(db_get_string(stmt), "%1s", dummy) != 1);
}

void error_handler(void *p)
{
    dbDriver *driver = (dbDriver *) p;
    
    db_close_database(driver);
    db_shutdown_driver(driver);
}
