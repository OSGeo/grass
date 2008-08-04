#include <grass/symbol.h>
#include <grass/colors.h>

FILE *open_vect(char *, char *);
int close_vect(FILE *);
int plot1(struct Map_info *, int, int, struct cat_list *,
	  const struct color_rgb *, const struct color_rgb *, int, SYMBOL *,
	  int, int, int, int, char *, int, char *, double, int, char *);
int label(struct Map_info *, int, int, struct cat_list *, LATTR *, int);
int topo(struct Map_info *, int, int, LATTR *);
int dir(struct Map_info *, int, struct cat_list *, int);
int darea(struct Map_info *, struct cat_list *, const struct color_rgb *,
	  const struct color_rgb *, int, int, int, int, struct Cell_head *,
	  char *, int, char *, double, int, char *);
int attr(struct Map_info *, int, char *, struct cat_list *, LATTR *, int);
int zcoor(struct Map_info *, int, LATTR *);
int test_bg_color(const char *);
void plot_polygon(double *, double *, int);
void plot_polyline(double *, double *, int);
