/* error.c */
void init_error();

/* connect.c */
int open_connection();
int close_connection();

/* cursor.c */
<<<<<<< HEAD
cursor *alloc_cursor(void);
=======
cursor *alloc_cursor();
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
void free_cursor(cursor *c);

/* describe.c */
int describe_table(SQLHSTMT stmt, dbTable **table);
