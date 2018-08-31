#include <grass/gis.h>
#include <grass/vector.h>

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
void trans2d(struct Map_info *, struct Map_info *, int,
             double, const char *, const char *);

/* trans3.c */
void trans3d(struct Map_info *, struct Map_info *, int,
             const char *, const char *);
