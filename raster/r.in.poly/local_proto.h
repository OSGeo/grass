#include <grass/raster.h>

/* get_item.c */
int get_item(FILE *, int, int *, int *, double *, double **, double **, int *,
	     struct Categories *);
/* getformat.c */
int getformat(FILE *, int, int *);

/* poly2rast.c */
int poly_to_rast(char *, char *, char *, int, int, int *);

/* raster.c */
int begin_rasterization(int, int);
int configure_plot(void);
int output_raster(int, int *);
int set_cat_int(int);
int set_cat_double(double);
int raster_dot(int, int);
