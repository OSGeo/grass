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
static int cmp(const void *, const void *);
static char *icon_files(void);

/* display.c */
int draw_line(int ltype, int line,
           const struct line_pnts *Points, const struct line_cats *Cats,
           int chcat, double size, int default_width,
              const struct cat_list *Clist, SYMBOL *Symb,
              RGBA_Color *primary_color, int *n_points, int *n_lines,
              int *n_centroids, int *n_boundaries, int *n_faces,
              RGBA_Color *secondary_color);

int display_lines(struct Map_info *, struct cat_list *, int,
          const char *, double, int, dbCatValArray *, double *, int,
          const struct color_rgb *, const struct color_rgb *);

/* legend.c */
void write_into_legend_file(const char *, const char *, const char *, double, double,
                            double *, int, int, struct color_rgb,
                            struct color_rgb *, int, int*, const char *);
