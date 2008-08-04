/* curses.c */
int Initialize_curses(void);
int Close_curses(void);
int Write_cats(struct Categories *, int);
int Write_menu(void);
int Write_status(int, int, int, int, int, int);
int Write_message(int, char *);
int Clear_message(void);
int Clear_menu(void);
int Write_menu_line(int, char *);
int Replot_screen(void);
int Get_curses_text(char[]);

/* get_info.c */
int get_map_info(char *, char *);

/* interact.c */
int interact(struct Categories *, struct Colors *, char *, char *);
int shift_color(int, int);

/* main.c */
int main(int, char **);

/* set_sigs.c */
int set_signals(void);

/* sigint.c */
void sigint(int);

/* tbl_toggle.c */
int table_toggle(char *, char *, struct Colors *);
