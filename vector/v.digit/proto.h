/* Driver */
int driver_open (void); 
int driver_close (void); 
int driver_refresh (void);
void driver_rgb_color(int r, int g, int b);
void driver_line_width(int w);
void driver_plot_line(double x1, double y1, double x2, double y2);
void driver_plot_icon(double x, double y, const char *icon);

/* Miscellaneous */
void tool_centre (void);
void next_tool(void);
void update (int, int);
void end (void);

/* Symbology */
int get_symb_code ( char *); 
char *get_symb_name ( int ); 
void symb_init ( void );
void symb_init_gui ( void );
void symb_set_driver_color ( int );

int symb_line_from_map ( int );
void symb_line_set_from_map ( int );
void symb_lines_init ( void );
void symb_updated_lines_set_from_map ( void );

int symb_node_from_map ( int );
void symb_node_set_from_map ( int );
void symb_nodes_init ( void );
void symb_updated_nodes_set_from_map ( void );

void updated_lines_and_nodes_erase_refresh_display(void); 

/* Edit */
int snap ( double *, double * );
void new_line (int);
void move_vertex (void);
void add_vertex (void);
void rm_vertex (void);
void split_line (void);
void move_line (void);
void delete_line (void);
void edit_line (void);

/* Attributes */
void copy_cats (void);
void display_cats (void);
int del_cat (int, int, int);
int add_cat (int, int, int);
void display_attributes (void);
int new_record (int, int);
int check_record (int, int);

/* Display */
void display_points ( struct line_pnts *, int );
void display_icon ( double, double, int, double, int, int);
void display_line ( int, int, int );
void display_updated_lines ( int );
void display_node ( int, int, int);
void display_updated_nodes ( int );
void display_map ( void );
void display_bg ( void );
void display_erase ( void );
void display_redraw ( void );

/* Zoom */
void zoom_window (void);
int zoom_centre (double factor);
void zoom_pan (void);
int zoom_default (void);
int zoom_region (void);

/* c_face.c */
int c_cancel (ClientData , Tcl_Interp *, int, char **);
int c_next_tool (ClientData , Tcl_Interp *, int, char **);
int c_tool_centre (ClientData , Tcl_Interp *, int, char **);
int c_set_color (ClientData , Tcl_Interp *, int, char **);
int c_set_on (ClientData , Tcl_Interp *, int, char **);
int c_create_table (ClientData , Tcl_Interp *, int, char **);
int c_table_definition (ClientData , Tcl_Interp *, int, char **);
int c_var_set (ClientData , Tcl_Interp *, int, char **);
int c_create_bgcmd (ClientData , Tcl_Interp *, int, char **);
int c_set_bgcmd (ClientData , Tcl_Interp *, int, char **);
int c_add_blank_bgcmd (ClientData , Tcl_Interp *, int, char **);
int c_del_cat (ClientData , Tcl_Interp *, int, char **);
int c_add_cat (ClientData , Tcl_Interp *, int, char **);

/* i_face.c */
int i_prompt (char *);
int i_prompt_buttons (char *, char *, char *);
int i_coor ( double, double);
int i_set_color ( char *, int, int, int);
int i_set_on ( char *, int);
int i_update (void);
void i_new_line_options ( int );
void i_set_cat_mode ( void );
void i_var_seti ( int, int ); 
void i_var_setd ( int, double ); 
void i_var_setc ( int, char* ); 
int i_message ( int, int, char*);
void i_add_bgcmd ( int );

/* Cats */
void cat_init ( void );
int cat_max_get ( int );
void cat_max_set ( int, int);

/* Variables */
void var_init ( void );
int var_seti ( int, int );
int var_setd ( int, double );
int var_setc ( int, char * );
int var_get_type_by_name ( char * );
int var_get_code_by_name ( char * );
char *var_get_name_by_code ( int code );
int var_geti ( int );
double var_getd ( int );
char *var_getc ( int );

/* Background */
int bg_add ( char *);

/* Utilities */
char *get_line_type_name ( int type);
void set_location(int x, int y);
void set_mode(int m);

typedef int tool_func_begin(void *closure);
typedef int tool_func_update(void *closure, int sxn, int syn, int button);
typedef int tool_func_end(void *closure);

void set_tool(tool_func_begin *begin,
	      tool_func_update *update,
	      tool_func_end *end,
	      void *closure);
void cancel_tool(void);
int c_update_tool (ClientData , Tcl_Interp *, int, char **);

/* form */
int reset_values(ClientData, Tcl_Interp *, int, char **);
int set_value(ClientData, Tcl_Interp *, int, char **);
int submit(ClientData, Tcl_Interp *, int, char **);
