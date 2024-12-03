#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

#include <gdal.h>
#include <gdal_version.h>
#include <ogr_api.h>
#include <cpl_string.h>

/* some hard limits */
#define SQL_BUFFER_SIZE 2000

struct Options {
    struct Option *input, *dsn, *layer, *type, *format, *field, *dsco, *lco,
        *otype, *method;
};

struct Flags {
    struct Flag *cat, *esristyle, *update, *nocat, *new, *append, *force2d,
        *multi, *list;
};

/* args.c */
void parse_args(int, char **, struct Options *, struct Flags *);

/* attrb.c */
int mk_att(int, struct field_info *, dbDriver *, int, int *, const char **, int,
           int, OGRFeatureH, int *);

/* attrb_fast.c */
int mk_att_fast(int, struct field_info *, int, int *, const char **, int, int,
                OGRFeatureH, int *, dbCursor *, int *, int *, int);

/* dsn.c */
char *get_datasource_name(const char *, int);

/* list.c */
char *OGR_list_write_drivers(void);
void list_formats(void);
char *default_driver(void);

/* create.c */
void create_ogr_layer(const char *, const char *, const char *, unsigned int,
                      char **, char **);
OGRwkbGeometryType get_multi_wkbtype(OGRwkbGeometryType);
OGRwkbGeometryType get_wkbtype(int, int);

/* export_lines.c */
int export_lines(struct Map_info *, int, int, int, int, int, OGRFeatureDefnH,
                 OGRLayerH, struct field_info *, dbDriver *, int, int *,
                 const char **, int, int, int *, int *);

/* export_lines_fast.c */
int export_lines_fast(struct Map_info *, int, int, int, int, int,
                      OGRFeatureDefnH, OGRLayerH, struct field_info *,
                      dbDriver *, int, int *, const char **, int, int, int *,
                      int *);

/* export_areas.c */
int export_areas(struct Map_info *, int, int, int, OGRFeatureDefnH, OGRLayerH,
                 struct field_info *, dbDriver *, int, int *, const char **,
                 int, int, int *, int *, int);

/* export_areas_fast.c */
int export_areas_fast(struct Map_info *, int, int, int, OGRFeatureDefnH,
                      OGRLayerH, struct field_info *, dbDriver *, int, int *,
                      const char **, int, int, int *, int *, int);
