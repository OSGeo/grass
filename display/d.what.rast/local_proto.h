/* opencell.c */
int opencell(char *, char *, char *);
/* show.c */
int show_cat(int, int, char *, char *, int, char *, int, char *, RASTER_MAP_TYPE);
int show_dval(int, int, char *, char *, DCELL, char *, int, char *);
int show_utm(char *, char *, double, double, struct Cell_head *, int, int, int, char *);
int show_buttons(int);
/* what.c */
int what(int, int, int, char *, int, int);
