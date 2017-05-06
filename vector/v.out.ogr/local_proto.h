#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

#include "ogr_api.h"
#include "cpl_string.h"


/* some hard limits */
#define SQL_BUFFER_SIZE 2000


struct Options {
    struct Option *input, *dsn, *layer, *type, *format,
	*field, *dsco, *lco, *otype;
};

struct Flags {
    struct Flag *cat, *esristyle, *update, *nocat, *new, *append, *force2d, *multi;
};

/* args.c */
void parse_args(int, char **,
		struct Options*, struct Flags *);

/* attributes.c */
int mk_att(int, struct field_info *, dbDriver *,
	   int, int *, const char **, int, int,
	   OGRFeatureH, int *);

/* dsn.c */
char *get_datasource_name(const char *, int);

/* list.c */
char *OGR_list_write_drivers();

/* create.c */
void create_ogr_layer(const char *, const char *, const char *,
		      unsigned int, char **, char **);
OGRwkbGeometryType get_multi_wkbtype(OGRwkbGeometryType);
OGRwkbGeometryType get_wkbtype(int, int);

/* export_lines.c */
int export_lines(struct Map_info *, int, int, int, int, int,
                 OGRFeatureDefnH, OGRLayerH,
                 struct field_info *, dbDriver *, int, int *, 
                 const char **, int, int,
                 int *, int *);

/* export_areas.c */
int export_areas(struct Map_info *, int, int, int, 
                 OGRFeatureDefnH, OGRLayerH,
                 struct field_info *, dbDriver *, int, int *, 
                 const char **, int, int,
                 int *, int *);
