#include <grass/dbmi.h>
#include <grass/symbol.h>
#include <grass/colors.h>

FILE *open_vect(char *, char *);
int close_vect(FILE *);
int plot1(struct Map_info *, int, int, struct cat_list *,
	  const struct color_rgb *, const struct color_rgb *, int, SYMBOL *,
	  int, int, int, int, char *, int, char *, double);
int dareatheme(struct Map_info *, struct cat_list *, dbCatValArray *,
	       double *, int, const struct color_rgb *,
	       const struct color_rgb *, int, struct Cell_head *, int);

int dcmp(const void *, const void *);
