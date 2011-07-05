/* start */
int start_mon(const char *, const char *, int);

/* select.c */
int select_mon(const char *);

/* stop.c */
int stop_mon(const char *);

/* list.c */
void list_mon();
void print_list(FILE *);
int check_mon(const char *);
