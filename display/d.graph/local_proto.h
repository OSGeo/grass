/* allocation.c */
char *falloc(int, int);
char *frealloc(char *, int, int, int);

/* do_graph.c */
int set_graph_stuff(void);
int set_text_size(void);
int do_draw(char *);
int do_move(char *);
int do_linewidth(char *);
int do_color(char *);
int do_poly(char *, FILE *);
int do_size(char *);
int do_rotate(char *);
int do_text(char *);
int check_alloc(int);
int do_icon(char *);
int do_symbol(char *);
void set_last_color(int, int, int, int);

/* graphics.c */
int graphics(FILE *);
