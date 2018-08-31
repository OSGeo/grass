/* error.c */
void init_error(void);

/* cursor.c */
cursor *alloc_cursor(void);
void free_cursor(cursor *);

/* describe.c */
int describe_table(PGresult *, dbTable **, cursor *);
int get_column_info(PGresult *, int, int *, int *, int *, int *);
int get_gpg_type(int);

/* parse.c */
int parse_conn(const char *, PGCONN *);
