#ifdef G_PROJ_MAIN
#  define G_PROJ_GLOBAL
#else
#  define G_PROJ_GLOBAL extern
#endif

G_PROJ_GLOBAL struct Key_Value *projinfo, *projunits;
G_PROJ_GLOBAL struct Cell_head cellhd;

/* input.c */
void input_currloc(void);
int input_wkt(char *);
int input_proj4(char *);
int input_epsg(int);
int input_georef(char *);

/* output.c */
void print_projinfo(void);
void print_datuminfo(void);
void print_proj4(int);
void print_wkt(int, int);
void create_location(char *, int);

/* datumtrans.c */
int set_datumtrans(int, int, int);
