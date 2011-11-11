int Box_abs(int, int, int, int);
int Box_rel(int, int);
int Color_table_float(void);
int Color_table_fixed(void);
int _get_lookup_for_color(int, int, int);
int get_table_type(void);
int Color(int);
int RGB_color(unsigned char, unsigned char, unsigned char);
int _get_color_index(int);
int _get_color_index_array(int *, int);
int Standard_color(int);
int Color_offset(int);
int get_color_offset(void);
int Cont_abs(int, int);
int Cont_rel(int, int);
int Erase(void);
int Font(char *);
int Get_text_box(char *, int *, int *, int *, int *);
int Linemod(char *);
int Move_abs(int, int);
int Get_current_xy(int *, int *);
int Move_rel(int, int);
int Number_of_colors(int *);
int Polydots_abs(int *, int *, int);
int Polydots_rel(int *, int *, int);
int Polyline_abs(int *, int *, int);
int Polyline_rel(int *, int *, int);
int Polygon_rel(int *, int *, int);
int Set_RGB_color(unsigned char, unsigned char, unsigned char);
int RGB_raster(int, int, register unsigned char *, register unsigned char *,
	       register unsigned char *, int);
int Raster_char(int, int, unsigned char *, int, int);
int Raster_int_def(int, int, int *, int, int);
int Raster_int(int, int, int *, int, int);
int Reset_colors(int, int, unsigned char *, unsigned char *, unsigned char *);
int Reset_color(unsigned char, unsigned char, unsigned char, int);
void close_mon(void);
int Set_window(int, int, int, int);
int window_clip(double *, double *, double *, double *);
int window_box_clip(double *, double *, double *, double *);
int Text(char *);
int Text_size(int, int);
int Text_rotation(float);
int clip(register double, register double, register double, register double,
	 register double *, register double *, register double *,
	 register double *);
int assign_fixed_color(int, int);
int get_fixed_color(int);
int get_fixed_color_array(register int *, register int);
int assign_standard_color(int, int);
int get_standard_color(int);
int get_max_std_colors(void);
int get_connection(char *, int *, int *);
int prepare_connection(void);
int check_connection(char *, char *;
		     int init_font(char *);
		     int get_char_vects(unsigned char, int *,
					unsigned char **, unsigned char **);
		     int drawchar(double, double, register double,
				  register double, char);
		     int soft_text_ext(int, int, double, double, double,
				       char *);
		     int get_text_ext(int *, int *, int *, int *);
		     int soft_text(int, int, double, double, double, char *);
		     int onechar(int, int, double, double, double,
				 register char);
