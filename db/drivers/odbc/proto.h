/* error.c */
void report_error ( char *err );

/* connect.c */
int open_connection ( );
int close_connection ( );

/* cursor.c */
cursor * alloc_cursor ();
void free_cursor ( cursor *c );

/* describe.c*/
int describe_table(SQLHSTMT   stmt, dbTable **table);
