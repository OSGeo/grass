/* analyze.c */
int analyze(void);

/* ask.c */
int ask_gis_files(char *, char *, char *, char *, int);

/* ask_mag.c */
int ask_magnification(int *);
int draw_mag(void);

/* call.c */
int call(int (*)(), char *);

/* cell.c */
int plotcell(int, int);

/* cellhd.c */
int Outline_cellhd(View *, struct Cell_head *);

/* colors.c */
int set_colors(struct Colors *);

/* conv.c */
int view_to_col(View *, int);
int view_to_row(View *, int);
int col_to_view(View *, int);
int row_to_view(View *, int);
double row_to_northing(struct Cell_head *, int, double);
double col_to_easting(struct Cell_head *, int, double);
double northing_to_row(struct Cell_head *, double);
double easting_to_col(struct Cell_head *, double);

/* curses.c */
int Begin_curses(void);
int End_curses(void);
int Suspend_curses(void);
int Resume_curses(void);
int Curses_allow_interrupts(int);
int Curses_clear_window(Window *);
int Curses_outline_window(Window *);
int Curses_write_window(Window *, int, int, char *);
int Curses_replot_screen(void);
int Curses_prompt_gets(char *, char *);
int Beep(void);
int Curses_getch(int);

/* debug.c */
int debug(char *);

/* digit.c */
int setup_digitizer(void);
int digitizer_point(double *, double *);

/* dot.c */
int dot(int, int);
int save_under_dot(int, int);
int restore_under_dot(void);
int release_under_dot(void);

/* drawcell.c */
int drawcell(View *);

/* driver.c */
int driver(void);

/* equ.c */
int Compute_equation(void);

/* find.c */
int find_target_files(void);

/* graphics.c */
int Init_graphics(void);
int Outline_box(int, int, int, int);
int Text_width(char *);
int Text(char *, int, int, int, int, int);
int Uparrow(int, int, int, int);
int Downarrow(int, int, int, int);

/* group.c */
int prepare_group_list(void);
int choose_groupfile(char *, char *);

/* input.c */
int Input_pointer(Objects *);
int Input_box(Objects *, int, int);
int Input_other(int (*)(), char *);
int Menu_msg(char *);
int Start_mouse_in_menu(void);

/* main.c */
int quit(int);
int error(const char *, int);

/* mark.c */
int mark(int, int, int);
int mark_point(View *, int, int);

/* mouse.c */
int Mouse_pointer(int *, int *, int *);
int Mouse_box_anchored(int, int, int *, int *, int *);
int Get_mouse_xy(int *, int *);
int Set_mouse_xy(int, int);

/* points.c */
int display_points(int);
int display_points_in_view(View *, int, double *, double *, int *, int);
int display_one_point(View *, double, double);

/* target.c */
int get_target(void);
int select_current_env(void);
int select_target_env(void);

/* title.c */
int display_title(View *);

/* view.c */
int Configure_view(View *, char *, char *, double, double);
int In_view(View *, int, int);
int Erase_view(View *);
double magnification(View *);

/* where.c */
int where(int, int);

/* zoom.c */
int zoom(void);

/* zoom_box.c */
int zoom_box(void);

/* zoom_pnt.c */
int zoom_point(void);
