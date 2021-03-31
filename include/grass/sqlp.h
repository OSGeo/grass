#ifndef GRASS_SQLP_H
#define GRASS_SQLP_H

/* SQL Parser */

/* KEYWORD OPS */

/* SQL COMMANDS */
#define SQLP_CREATE 1
#define SQLP_DROP   2
#define SQLP_INSERT 3
#define SQLP_SELECT 4
#define SQLP_UPDATE 5
#define SQLP_DELETE 6
#define SQLP_ADD_COLUMN 7
#define SQLP_DROP_COLUMN 8

/* SQL OPERATORS */
  /* Arithmetical */
#define SQLP_ADD   1		/* + */
#define SQLP_SUBTR 2		/* - */
#define SQLP_MLTP  3		/* * */
#define SQLP_DIV   4		/* / */

  /* Comparison */
#define SQLP_EQ   11		/* =  */
#define SQLP_LT   12		/* <  */
#define SQLP_LE   13		/* <= */
#define SQLP_GT   14		/* >  */
#define SQLP_GE   15		/* >= */
#define SQLP_NE   16		/* <> */
#define SQLP_MTCH 17		/* ~ */

#define SQLP_ISNULL  18		/* IS NULL */
#define SQLP_NOTNULL 19		/* IS NULL */

   /* Logical */
#define SQLP_AND  21
#define SQLP_OR   22
#define SQLP_NOT  23

/* SQL VALUE TYPES, NOT COLUMN TYPES */
#define SQLP_NULL 1		/* value NULL -> unknown type */
#define SQLP_S    2		/* string */
#define SQLP_I    3		/* integer */
#define SQLP_D    4		/* float */
#define SQLP_BOOL 5		/* used only for type of expression */
#define SQLP_EXPR  6		/* expression XXX */

/* SQL COLUMN TYPES */
#define SQLP_VARCHAR 1
#define SQLP_INTEGER 2
#define SQLP_DOUBLE  3
#define SQLP_DATE    4
#define SQLP_TIME    5

#define SQLP_MAX_TABLE  200
#define SQLP_MAX_ERR    500

/* Condition node */
#define SQLP_NODE_COLUMN     1
#define SQLP_NODE_VALUE      2
#define SQLP_NODE_EXPRESSION 3

/* Order direction */
#define SORT_ASC  1
#define SORT_DESC 2


typedef struct
{
    int type;			/* SQLP_S, SQLP_I, SQLP_D, SQLP_NULL, SQL_EXPR */
    char *s;			/* pointer to string or NULL */
    int i;
    double d;
    struct sqlpnode *expr;
} SQLPVALUE;

typedef struct sqlpnode
{
    int node_type;		/* Node type: SQLP_NODE_COLUMN, SQLP_NODE_VALUE, SQLP_NODE_EXPRESSION */
    int oper;			/* Operator code */
    struct sqlpnode *left;	/* left argument, sometimes NULL */
    struct sqlpnode *right;	/* right argument, sometimes NULL */
    char *column_name;
    SQLPVALUE value;
} SQLPNODE;

typedef struct
{
    char *stmt;			/* input statement string */
    char *cur;			/* cursor for parser */
    char errmsg[SQLP_MAX_ERR + 1];
    int command;
    char table[SQLP_MAX_TABLE + 1];
    SQLPVALUE *Col;		/* column names */
    int *ColType;
    int *ColWidth;		/* length */
    int *ColDecim;		/* decimals */
    int aCol;			/* allocated */
    int nCol;			/* number of columns */
    SQLPVALUE *Val;		/* values */
    int aVal;
    int nVal;
    SQLPNODE *upperNodeptr;
    char *orderCol;		/* column name which should be used for sorting (ORDER BY) or NULL (no sorting) */
    int orderDir;		/* direction of ordering (ASC or DESC) */
} SQLPSTMT;

#include <grass/defs/sqlp.h>

extern SQLPSTMT *sqlpStmt;

#endif
