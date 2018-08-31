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

%{
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/sqlp.h>

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

%}
	
	/* symbolic tokens */

%union {
	int    intval;
	double floatval;
	char   *strval;
	int    subtok;
	SQLPNODE   *node;
}

	/* operators */
%type <node>	y_column
%type <node>	y_value
%type <node>	y_atom
%type <node>	y_term
%type <node>	y_product
%type <node>	y_expression
%type <node>	y_comparison
%type <node>	y_boolean
%type <node>	y_sub_condition2
%type <node>	y_sub_condition
%type <node>	y_condition

	/* literal keyword tokens */
%token <strval> COMPARISON_OPERATOR
%token <strval> NAME
%token <strval> STRING
%token <intval> INTNUM 
%token <floatval> FLOATNUM

%token ADD
%token DROP
%token COLUMN
%token EQUAL
%token SELECT FROM WHERE
%token DELETE
%token INSERT INTO VALUES
%token UPDATE SET
%token AND
%token OR
%token NOT
%token ALTER TABLE
%token CREATE
%token NULL_VALUE
%token VARCHAR
%token INT
%token INTEGER
%token DOUBLE
%token PRECISION
%token DATE
%token TIME
%token ORDER BY
%token IS
%token ASC
%token DESC

%{
 
extern int yylex(void);

%}

%%

y_sql:	
		y_alter
	|	y_create
	|	y_drop
	|	y_insert
	|	y_select
	|	y_update
	|	y_delete
	|	y_sql ';'
	;
	
y_alter:
		ALTER TABLE y_table ADD COLUMN y_columndef	{ sqpCommand(SQLP_ADD_COLUMN); }
	|	ALTER TABLE y_table ADD y_columndef		{ sqpCommand(SQLP_ADD_COLUMN); }
	|	ALTER TABLE y_table DROP COLUMN NAME            { sqpCommand(SQLP_DROP_COLUMN); sqpColumn($6);}
	;
	
y_create:
		CREATE TABLE y_table '(' y_columndefs ')'	{ sqpCommand(SQLP_CREATE); }
	;
	
y_drop:
		DROP TABLE y_table				{ sqpCommand(SQLP_DROP); }
	;

y_select:
		SELECT y_columns FROM y_table			{ sqpCommand(SQLP_SELECT); }
	|	SELECT y_columns FROM y_table WHERE y_condition	{ sqpCommand(SQLP_SELECT); }
	|	SELECT y_columns FROM y_table ORDER BY y_order	{ sqpCommand(SQLP_SELECT); }
	|	SELECT y_columns FROM y_table WHERE y_condition ORDER BY y_order { sqpCommand(SQLP_SELECT); }
	;
	
y_delete:
		DELETE FROM y_table				{ sqpCommand(SQLP_DELETE); }
        |	DELETE FROM y_table WHERE y_condition		{ sqpCommand(SQLP_DELETE); }
	;

y_insert:
		INSERT INTO y_table y_values			{ sqpCommand(SQLP_INSERT); }
        |	INSERT INTO y_table '(' y_columns ')' y_values	{ sqpCommand(SQLP_INSERT); }
	;

y_update:
		UPDATE y_table SET y_assignments		{ sqpCommand(SQLP_UPDATE); }
	|	UPDATE y_table SET y_assignments WHERE y_condition	{ sqpCommand(SQLP_UPDATE); }

	;
	
y_columndefs:
		y_columndef
	|	y_columndefs ',' y_columndef
	;

y_columndef:
		NAME VARCHAR '(' INTNUM ')'	{ sqpColumnDef( $1, SQLP_VARCHAR, $4, 0 ); }
	|	NAME INT 			{ sqpColumnDef( $1, SQLP_INTEGER,  0, 0 ); }
	|	NAME INTEGER 			{ sqpColumnDef( $1, SQLP_INTEGER,  0, 0 ); }
	|	NAME DOUBLE			{ sqpColumnDef( $1, SQLP_DOUBLE,   0, 0 ); }
	|	NAME DOUBLE PRECISION		{ sqpColumnDef( $1, SQLP_DOUBLE,   0, 0 ); }
	|	NAME DATE			{ sqpColumnDef( $1, SQLP_DATE,     0, 0 ); }
	|	NAME TIME			{ sqpColumnDef( $1, SQLP_TIME,     0, 0 ); }
	;

y_columns:
	'*'
        |	y_column_list
	;
	
y_column_list:
		NAME				{ sqpColumn( $1 ); }
	|	y_column_list ',' NAME		{ sqpColumn( $3 ); }
	;

y_table:
		NAME 				{ sqpTable( $1 ); }
	;
	
y_values:
		VALUES '(' y_value_list ')'
	;

y_value_list:
		NULL_VALUE			{ sqpValue( NULL,  0, 0.0, SQLP_NULL ); }
	|	STRING				{ sqpValue( $1,    0, 0.0, SQLP_S ); }
        |	INTNUM				{ sqpValue( NULL, $1, 0.0, SQLP_I ); }
        |      '-' INTNUM 			{ sqpValue( NULL, -$2, 0.0, SQLP_I ); }
	|	FLOATNUM			{ sqpValue( NULL,  0,  $1, SQLP_D ); }
	| 	'-' FLOATNUM 			{ sqpValue( NULL, 0, -$2, SQLP_D ); }
	|	y_value_list ',' NULL_VALUE	{ sqpValue( NULL,  0, 0.0, SQLP_NULL ); }
	|	y_value_list ',' STRING		{ sqpValue( $3,    0, 0.0, SQLP_S ); }
	|	y_value_list ',' INTNUM		{ sqpValue( NULL, $3, 0.0, SQLP_I ); }
	| 	y_value_list ',' '-' INTNUM 	{ sqpValue( NULL, -$4, 0.0, SQLP_I ); }
	|	y_value_list ',' FLOATNUM	{ sqpValue( NULL,  0,  $3, SQLP_D ); }
	| 	y_value_list ',' '-' FLOATNUM 	{ sqpValue( NULL, 0, -$4, SQLP_D ); }
	;

y_assignments:
		y_assignment
	|	y_assignments ',' y_assignment
	;
	
y_assignment:
                NAME EQUAL NULL_VALUE	{ sqpAssignment( $1, NULL,  0, 0.0, NULL, SQLP_NULL ); }
/*        |	NAME EQUAL STRING	{ sqpAssignment( $1,   $3,  0, 0.0, NULL, SQLP_S ); }
        |	NAME EQUAL INTNUM	{ sqpAssignment( $1, NULL, $3, 0.0, NULL, SQLP_I ); }
        |	NAME EQUAL FLOATNUM	{ sqpAssignment( $1, NULL,  0,  $3, NULL, SQLP_D ); }
*/        |       NAME EQUAL y_expression { sqpAssignment( $1, NULL, 0, 0.0, $3, SQLP_EXPR ); }
	;

y_condition:	
		y_sub_condition { 
		    $$ = $1;
		    sqlpStmt->upperNodeptr = $$; 
		}	
	;

y_sub_condition:	
		y_sub_condition2 { $$ = $1; }
	|	y_sub_condition OR y_sub_condition2 { $$ = sqpNewExpressionNode (SQLP_OR, $1, $3); }
	;

y_sub_condition2:	
		y_boolean { $$ = $1; }
	|	y_sub_condition2 AND y_boolean { $$ = sqpNewExpressionNode (SQLP_AND, $1, $3); }
	;

y_boolean:	
		y_comparison { $$ = $1; }
	|	'(' y_sub_condition ')' { $$ = $2; }
	|	NOT y_boolean { $$ = sqpNewExpressionNode ( SQLP_NOT, NULL, $2); }
	;

/* Note EQUAL should be one of COMPARISON but there is maybe some reason ... */
y_comparison:
		y_expression EQUAL y_expression {
		    $$ = sqpNewExpressionNode ( SQLP_EQ, $1, $3);
		}
	|	y_expression COMPARISON_OPERATOR y_expression {
		    $$ = sqpNewExpressionNode ( sqpOperatorCode($2), $1, $3);
		}
	|	y_expression IS NULL_VALUE {
		    $$ = sqpNewExpressionNode ( SQLP_ISNULL, NULL, $1);
		}
	|	y_expression NOT NULL_VALUE {
		    $$ = sqpNewExpressionNode ( SQLP_NOTNULL, NULL, $1);
		}
	;	

/* Mathematical expression */
y_expression:
		y_product			{ $$ = $1; }
	|	y_expression '+' y_product {
		    $$ = sqpNewExpressionNode ( sqpOperatorCode("+"), $1, $3 );
		}
	|	y_expression '-' y_product {
		    $$ = sqpNewExpressionNode ( sqpOperatorCode("-"), $1, $3 );
		}
	;

y_product:
		y_term				{ $$ = $1; }
	|	y_product '*' y_term {
		    $$ = sqpNewExpressionNode ( sqpOperatorCode("*"), $1, $3 );
		}
	|	y_product '/' y_term {
		    $$ = sqpNewExpressionNode ( sqpOperatorCode("/"), $1, $3 );
		}
	;

y_term:
		y_atom				{ $$ = $1; }
	|	'-' y_term {
		    $$ = sqpNewExpressionNode ( sqpOperatorCode("-"), sqpNewValueNode ( NULL, 0, 0.0,  SQLP_I ), $2 );
		}
	;

y_atom:
		y_value				{ $$ = $1; }
	|	y_column			{ $$ = $1; }
	|	'(' y_expression ')'		{ $$ = $2; }
	;

/* Value used in expressions */ 
y_value:
		STRING				{ $$ = sqpNewValueNode (   $1,  0, 0.0,  SQLP_S ); }
	|	INTNUM				{ $$ = sqpNewValueNode ( NULL, $1, 0.0,  SQLP_I ); }
	|	FLOATNUM			{ $$ = sqpNewValueNode ( NULL,  0,  $1,  SQLP_D ); }
	;

/* Column used in expressions */
y_column:
		NAME				{$$ = sqpNewColumnNode (  $1 );}
	;

y_order: y_order_asc | y_order_desc;

y_order_asc:
		NAME 				{ sqpOrderColumn( $1, SORT_ASC ); }
	|	NAME ASC 			{ sqpOrderColumn( $1, SORT_ASC ); }
	;
y_order_desc:
		NAME DESC			{ sqpOrderColumn( $1, SORT_DESC ); }
	;
%%


