/* error.c */
void init_error(void);
void append_error(const char *);
void report_error(void);

/* cursor.c */
cursor *alloc_cursor();
void free_cursor(cursor *);

/* describe.c */
int describe_table(MYSQL_RES *, dbTable **, cursor *);
void field_info(MYSQL_FIELD *, int *, int *);

/* parse.c */
int parse_conn(char *, CONNPAR *);
