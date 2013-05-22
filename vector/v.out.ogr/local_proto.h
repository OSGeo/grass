#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

#include "ogr_api.h"
#include "cpl_string.h"


/* some hard limits */
#define SQL_BUFFER_SIZE 2000


struct Options {
    struct Option *input, *dsn, *layer, *type, *format,
	*field, *dsco, *lco;
};

struct Flags {
    struct Flag *cat, *esristyle, *poly, *update, *nocat, *new, *append, *force2d;
};

/* args.c */
void parse_args(int, char **,
		struct Options*, struct Flags *);

/* attributes.c */
int mk_att(int cat, struct field_info *Fi, dbDriver *Driver,
	   int ncol, int *colctype, const char **colname, int doatt, int nocat,
	   OGRFeatureH Ogr_feature, int *, int *);

/* list.c */
char *OGR_list_write_drivers();

/* create.c */
void create_ogr_layer(const char *, const char *, const char *,
		      unsigned int, char **, char **);
