#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

#include "plot.h"

int test_bg_color(const char *);

/* attr.c */
int display_attr(struct Map_info *, int, char *, struct cat_list *, LATTR *, int);

/* area.c */
int display_area(struct Map_info *, struct cat_list *, const struct Cell_head *,
		 const struct color_rgb *, const struct color_rgb *, int, int,
		 int, int, const char *,
		 int, const char *, double,
		 int, const char *,
		 dbCatValArray *, dbCatValArray *, int);

/* dir.c */
int display_dir(struct Map_info *, int, struct cat_list *, int);

/* labels.c */
int display_label(struct Map_info *, int, struct cat_list *, LATTR *, int);
void show_label(double *, double *, LATTR *, const char *);
void show_label_line(const struct line_pnts *, int, LATTR *, const char *);

/* lines.c */
int display_lines(struct Map_info *, int, struct cat_list *,
		  const struct color_rgb *, const struct color_rgb *, int,
		  const char *, double, const char *, int, const char *,
		  int, int, int, const char *,
		  int, const char *, double,
		  int, const char *,
		  dbCatValArray *, dbCatValArray *, int,
		  dbCatValArray *, int, dbCatValArray *, int);

/* shape.c */
int display_shape(struct Map_info *, int, struct cat_list *, const struct Cell_head *, 
		  const struct color_rgb *, const struct color_rgb *, int,
		  const char *, double, const char *, int, const char *, /* lines only */
		  int, int, int, char *,
		  int, char *, double,
		  int, char *);

/* opt.c */
int option_to_display(const struct Option *);
void options_to_lattr(LATTR *, const char *,
		      const char *, const char *, const char *,
		      int, const char *, const char *,
		      const char *, const char *);
int option_to_color(struct color_rgb *, const char *);
void option_to_where(struct Map_info *, struct cat_list *, const char *);

/* topo.c */
int display_topo(struct Map_info *, int, LATTR *);

/* zcoor.c */
int display_zcoor(struct Map_info *, int, LATTR *);
