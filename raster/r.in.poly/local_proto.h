/* get_item.c */
int get_item(FILE *, int *, long *, double **, double **, int *,
	     struct Categories *);
/* getformat.c */
int getformat(FILE *);

/* poly2rast.c */
int poly_to_rast(char *, char *, char *, int);

/* raster.c */
int begin_rasterization(int, int);
int configure_plot(void);
int output_raster(int);
int set_cat(long);
int raster_dot(int, int);
