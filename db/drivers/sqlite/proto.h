#ifndef __SQLITE_PROTO_H__
#define __SQLITE_PROTO_H__

/* error.c */
void init_error ( void );
void append_error ( char * );
void report_error ( void );

/* cursor.c */
cursor * alloc_cursor ();
void free_cursor ( cursor * );

/* describe.c*/
int describe_table ( sqlite3_stmt *, dbTable **, cursor * );
void get_column_info ( sqlite3_stmt *, int, int *, int * );

#endif /* __SQLITE_PROTO_H__ */
