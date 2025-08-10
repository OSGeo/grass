#ifndef V_EXTERNAL_LOCAL_PROTO_H
#define V_EXTERNAL_LOCAL_PROTO_H

#include <gdal.h>
#include <gdal_version.h>
#include <ogr_api.h>

struct _options {
    struct Option *dsn, *output, *layer, *where;
};

struct _flags {
    struct Flag *format, *layer, *tlist, *topo, *list, *override, *proj;
};

/* args.c */
void parse_args(int, char **, struct _options *, struct _flags *);

/* dsn.c */
char *get_datasource_name(const char *, int);

/* list.c */
void list_formats(void);
int list_layers(FILE *, const char *, char **, int, int);
void get_table_name(const char *, char **, char **);

/* proj.c */
void check_projection(struct Cell_head *, GDALDatasetH, int, char *, char *,
                      int, int, int);
#endif
