#include <grass/gis.h>
#include <grass/Vect.h>

struct opts {
  struct Flag *reverse, *table;
  
  struct Option *input, *output;
  struct Option *type;
  struct Option *height;
  struct Option *field, *column;
};

/* args.c */
void parse_args(struct opts *);

/* trans2.c */
int trans2d(struct Map_info *, struct Map_info *, int,
	    double, int, const char *);

/* trans3.c */
int trans3d(struct Map_info *, struct Map_info *, int,
	    int, const char *);
