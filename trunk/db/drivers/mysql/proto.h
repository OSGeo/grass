/* error.c */
void init_error(void);

/* cursor.c */
cursor *alloc_cursor();
void free_cursor(cursor *);

/* describe.c */
int describe_table(MYSQL_RES *, dbTable **, cursor *);
void field_info(MYSQL_FIELD *, int *, int *);

/* parse.c */
int parse_conn(const char *, CONNPAR *);

/* replace.c */
int replace_variables(char *, char **, char **);
