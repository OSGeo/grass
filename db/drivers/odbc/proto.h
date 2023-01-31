/* error.c */
void init_error();

/* connect.c */
int open_connection();
int close_connection();

/* cursor.c */
<<<<<<< HEAD
cursor *alloc_cursor();
=======
cursor *alloc_cursor(void);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
void free_cursor(cursor *c);

/* describe.c */
int describe_table(SQLHSTMT stmt, dbTable **table);
