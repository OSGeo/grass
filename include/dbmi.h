/*!
  \file include/dbmi.h

  \brief Main header of \ref dbmilib

  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
*/

#ifndef GRASS_DBMI_H
#define GRASS_DBMI_H

#include <stdio.h>
#include <grass/gis.h>

#define DB_VERSION "0"

#ifdef HAVE_SQLITE
#define DB_DEFAULT_DRIVER "sqlite"
#else
#define DB_DEFAULT_DRIVER "dbf"
#endif

/* DB Prodedure Numbers */
#define DB_PROC_VERSION                 999

#define DB_PROC_CLOSE_DATABASE		101
#define DB_PROC_CREATE_DATABASE		102
#define DB_PROC_DELETE_DATABASE		103
#define DB_PROC_FIND_DATABASE		104
#define DB_PROC_LIST_DATABASES		105
#define DB_PROC_OPEN_DATABASE		106
#define DB_PROC_SHUTDOWN_DRIVER		107

#define DB_PROC_CLOSE_CURSOR		201
#define DB_PROC_DELETE			202
#define DB_PROC_FETCH			203
#define DB_PROC_INSERT			204
#define DB_PROC_OPEN_INSERT_CURSOR	205
#define DB_PROC_OPEN_SELECT_CURSOR	206
#define DB_PROC_OPEN_UPDATE_CURSOR	207
#define DB_PROC_UPDATE			208
#define DB_PROC_ROWS			209
#define DB_PROC_BIND_UPDATE		220
#define DB_PROC_BIND_INSERT		221

#define DB_PROC_EXECUTE_IMMEDIATE	301
#define DB_PROC_BEGIN_TRANSACTION	302
#define DB_PROC_COMMIT_TRANSACTION	303

#define DB_PROC_CREATE_TABLE		401
#define DB_PROC_DESCRIBE_TABLE		402
#define DB_PROC_DROP_TABLE		403
#define DB_PROC_LIST_TABLES		404
#define DB_PROC_ADD_COLUMN		405
#define DB_PROC_DROP_COLUMN		406
#define DB_PROC_GRANT_ON_TABLE		407

#define DB_PROC_CREATE_INDEX		701
#define DB_PROC_LIST_INDEXES		702
#define DB_PROC_DROP_INDEX		703

/* Unix file permissions */
#define DB_PERM_R	01
#define DB_PERM_W	02
#define DB_PERM_X	04

/* DB Error codes */
#define DB_OK		 0
#define DB_FAILED	 1
#define DB_NOPROC	 2
#define DB_MEMORY_ERR	-1
#define DB_PROTOCOL_ERR	-2
#define DB_EOF		-1

/* dbColumn.sqlDataType */
#define DB_SQL_TYPE_UNKNOWN          0

#define DB_SQL_TYPE_CHARACTER        1
#define DB_SQL_TYPE_SMALLINT         2
#define DB_SQL_TYPE_INTEGER          3
#define DB_SQL_TYPE_REAL             4
#define DB_SQL_TYPE_DOUBLE_PRECISION 6
#define DB_SQL_TYPE_DECIMAL          7
#define DB_SQL_TYPE_NUMERIC          8
#define DB_SQL_TYPE_DATE             9
#define DB_SQL_TYPE_TIME            10
#define DB_SQL_TYPE_TIMESTAMP       11
#define DB_SQL_TYPE_INTERVAL        12
#define DB_SQL_TYPE_TEXT            13	/* length not defined */

#define DB_SQL_TYPE_SERIAL          21

/* these are OR'ed (|) with the TIMESTAMP and INTERVAL type */
#define DB_YEAR              0x4000
#define DB_MONTH             0x2000
#define DB_DAY               0x1000
#define DB_HOUR              0x0800
#define DB_MINUTE            0x0400
#define DB_SECOND            0x0200
#define DB_FRACTION          0x0100
#define DB_DATETIME_MASK     0xFF00

/* dbColumn.CDataType */
#define DB_C_TYPE_STRING          1
#define DB_C_TYPE_INT             2
#define DB_C_TYPE_DOUBLE          3
#define DB_C_TYPE_DATETIME        4

/* fetch positions */
#define DB_CURRENT		1
#define DB_NEXT			2
#define DB_PREVIOUS		3
#define DB_FIRST		4
#define DB_LAST			5

/* cursor modes/types */
#define DB_READONLY		1
#define DB_INSERT		2
#define DB_UPDATE		3
#define DB_SEQUENTIAL		0
#define DB_SCROLL		1
#define DB_INSENSITIVE		4

/* privilege modes */
#define DB_GRANTED		1
#define DB_NOT_GRANTED		-1

/* Privileges */
#define DB_PRIV_SELECT       0x01

#define DB_GROUP             0x01
#define DB_PUBLIC            0x02

/* default value modes */
#define DB_DEFINED	1
#define DB_UNDEFINED	2

/* static buffer for SQL statements */
#define DB_SQL_MAX      4096

typedef void *dbAddress;
typedef int dbToken;

typedef struct _db_string
{
    char *string;
    int nalloc;
} dbString;

typedef struct _dbmscap
{
    char driverName[256];	/* symbolic name for the dbms system */
    char startup[256];		/* command to run the driver */
    char comment[256];		/* comment field             */
    struct _dbmscap *next;	/* linked list               */
} dbDbmscap;

typedef struct _db_dirent
{
    dbString name;		/* file/dir name             */
    int isdir;			/* bool: name is a directory */
    int perm;			/* permissions               */
} dbDirent;

typedef struct _db_driver
{
    dbDbmscap dbmscap;		/* dbmscap entry for this driver */
    FILE *send, *recv;		/* i/o to-from driver            */
    int pid;			/* process id of the driver      */
} dbDriver;

typedef struct _db_handle
{
    dbString dbName;		/* database name               */
    /* dbString dbPath; *//* directory containing dbName */
    dbString dbSchema;		/* database schema */
} dbHandle;

typedef struct _db_date_time
{
    char current;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    double seconds;
} dbDateTime;

typedef struct _db_value
{
    char isNull;
    int i;
    double d;
    dbString s;
    dbDateTime t;
} dbValue;

typedef struct _db_column
{
    dbString columnName;
    dbString description;
    int sqlDataType;
    int hostDataType;
    dbValue value;
    int dataLen;
    int precision;
    int scale;
    char nullAllowed;
    char hasDefaultValue;
    char useDefaultValue;
    dbValue defaultValue;
    int select;
    int update;
} dbColumn;

typedef struct _db_table
{
    dbString tableName;
    dbString description;
    int numColumns;
    dbColumn *columns;
    int priv_insert;
    int priv_delete;
} dbTable;

typedef struct _db_cursor
{
    dbToken token;
    dbDriver *driver;
    dbTable *table;
    short *column_flags;
    int type;
    int mode;
} dbCursor;

typedef struct _db_index
{
    dbString indexName;
    dbString tableName;
    int numColumns;
    dbString *columnNames;
    char unique;
} dbIndex;

typedef struct _db_driver_state
{
    char *dbname;
    char *dbschema;
    int open;
    int ncursors;
    dbCursor **cursor_list;
} dbDriverState;

/* category value (integer) */
typedef struct
{
    int cat;			/* category */
    int val;			/* value */
} dbCatValI;

/* category value */
typedef struct
{
    int cat;			/* category */
    int isNull;
    union
    {
	int i;
	double d;
	/* s and t were added 22.8.2005, both are pointers,
	 * they so should not take more than 8 bytes.
	 * It would be better to add dbString, not pointer,
	 * But it could be > 8 bytes on some systems */
	dbString *s;
	dbDateTime *t;
    } val;
} dbCatVal;

/* category value array */
typedef struct
{
    int n_values;
    int alloc;
    int ctype;			/* C type of values stored in array DB_C_TYPE_* */
    dbCatVal *value;
} dbCatValArray;

/* parameters of connection */
typedef struct _db_connection
{
    char *driverName;
    /* char *hostName; */
    char *databaseName;
    char *schemaName;
    char *location;
    char *user;
    char *password;
    char *keycol;		/* name of default key column */
    char *group;		/* deafault group to which select privilege is granted */
} dbConnection;

/* reclass rule */
typedef struct
{
    int count;			/* number of defined rules */
    int alloc;			/* size of allocated array */
    char *table;		/* table name */
    char *key;			/* key column name */
    int *cat;			/* array of new category numbers */
    char **where;		/* array of SQL WHERE conditions */
    char **label;		/* array of new category labels */
} dbRclsRule;

#include <grass/defs/dbmi.h>

#endif
