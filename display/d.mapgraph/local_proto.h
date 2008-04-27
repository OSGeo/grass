/* do_graph.c */
int set_text_size(void);
int do_draw(char *);
int do_move(char *);
int do_icon(char *);
int do_color(char *);
int do_poly(char *, int);
int do_size(char *);
int do_text(char *);
int check_alloc(int);
int scan_en(char *, double *, double *, int);
/* graphics.c */
int graphics(void);
/* read_line.c */
int read_line(char *, int);
int bad_line(char *);
