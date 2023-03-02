#ifndef V_EXTERNAL_LOCAL_PROTO_H
#define V_EXTERNAL_LOCAL_PROTO_H

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <gdal.h>
#include <gdal_version.h>
#include <ogr_api.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/* define type of input datasource
 * as of GDAL 2.2, all functions having as argument a GDAL/OGR dataset
 * must use the GDAL version, not the OGR version */
#if GDAL_VERSION_NUM >= 2020000
typedef GDALDatasetH ds_t;

#define ds_getlayerbyindex(ds, i) GDALDatasetGetLayer((ds), (i))
#define ds_close(ds)              GDALClose(ds)
#else
typedef OGRDataSourceH ds_t;

#define ds_getlayerbyindex(ds, i) OGR_DS_GetLayer((ds), (i))
#define ds_close(ds)              OGR_DS_Destroy(ds)
#endif

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
void check_projection(struct Cell_head *, ds_t, int, char *, char *, int, int,
                      int);
#endif
