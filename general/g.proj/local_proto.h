extern struct Key_Value *projinfo, *projunits;
extern struct Cell_head cellhd;

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
void create_location(char *);

/* datumtrans.c */
int set_datumtrans(int, int);
