/* error.c */
void init_error();

/* connect.c */
int open_connection();
int close_connection();

/* cursor.c */
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
cursor *alloc_cursor(void);
=======
cursor *alloc_cursor();
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
cursor *alloc_cursor(void);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
cursor *alloc_cursor();
=======
cursor *alloc_cursor(void);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
void free_cursor(cursor *c);

/* describe.c */
int describe_table(SQLHSTMT stmt, dbTable **table);
