#ifndef GRASS_SQLPDEFS_H
#define GRASS_SQLPDEFS_H

int my_yyinput(char *buf, int max_size);
void yyerror(const char *s);
int yyparse();
int yywrap();

int sqpSaveStr(SQLPVALUE * st, char *c);
void sqpInitValue(SQLPVALUE * val);
void sqpCopyValue(SQLPVALUE * from, SQLPVALUE * to);

SQLPSTMT *sqpInitStmt(void);
int sqpFreeStmt(SQLPSTMT * st);
int sqpPrintStmt(SQLPSTMT * st);
int sqpAllocCol(SQLPSTMT * st, int n);
int sqpAllocVal(SQLPSTMT * st, int n);
int sqpAllocCom(SQLPSTMT * st, int n);
int sqpInitParser(SQLPSTMT * st);


void sqpCommand(int command);
void sqpTable(char *table);
void sqpColumn(char *column);
void sqpColumnDef(char *column, int type, int width, int decimals);
void sqpValue(char *strval, int intval, double dblval, int type);
void sqpAssignment(char *column, char *strval, int intval, double dblval,
		   SQLPNODE * expr, int type);
void sqpOrderColumn(char *col, int dir);
int sqpOperatorCode(char *);
char *sqpOperatorName(int);

SQLPNODE *sqpNewNode(void);

SQLPNODE *sqpNewExpressionNode(int oper, SQLPNODE * left, SQLPNODE * right);
SQLPNODE *sqpNewColumnNode(char *name);
SQLPNODE *sqpNewValueNode(char *strval, int intval, double dblval, int type);

void sqpFreeNode(SQLPNODE *);

#endif
