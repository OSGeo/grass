/* allocation.c */
void *falloc(int, int);
void *frealloc(void *, int, int);

/* do_graph.c */
int set_graph_stuff(void);
int set_text_size(void);
int do_draw(const char *);
int do_move(const char *);
int do_linewidth(const char *);
int do_color(const char *);
int do_poly(char *, FILE *);
int do_size(const char *);
int do_rotate(const char *);
int do_text(const char *);
int check_alloc(int);
int do_icon(const char *);
int do_symbol(const char *);
void set_last_color(int, int, int, int);

/* graphics.c */
int graphics(FILE *);
