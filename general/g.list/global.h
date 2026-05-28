#include <stdio.h>
#include <grass/gis.h>
#include <grass/gjson.h>
#include <grass/manage.h>

enum { TYPE_RAST, TYPE_RAST3D, TYPE_VECT, TYPE_OTHERS };

enum OutputFormat { PLAIN, JSON, SHELL };

struct elist {
    char *type;
    char *name;
    char *mapset;
};

void make_list(struct elist **, int *, int *, const struct list *, const char *,
               const struct Cell_head *);
void print_list(FILE *, struct elist *, int, const char *, int, int,
                enum OutputFormat);
