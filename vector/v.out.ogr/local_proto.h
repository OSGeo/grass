#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

#include <gdal.h>
#include <gdal_version.h>
#include <ogr_api.h>
#include <cpl_string.h>

/* switch to new GDAL API with GDAL 2.2+ */
#if GDAL_VERSION_NUM >= 2020000
typedef GDALDatasetH ds_t;
typedef GDALDriverH dr_t;
#define get_driver_by_name		GDALGetDriverByName
#define get_driver			GDALGetDriver
#define ds_getlayerbyindex(ds, i)	GDALDatasetGetLayer((ds), (i))
#define ds_close(ds)			GDALClose(ds)
#else
typedef OGRDataSourceH ds_t;
typedef OGRSFDriverH dr_t;
#define get_driver_by_name		OGRGetDriverByName
#define get_driver			OGRGetDriver
#define ds_getlayerbyindex(ds, i)	OGR_DS_GetLayer((ds), (i))
#define ds_close(ds)			OGR_DS_Destroy(ds)
#endif

/* some hard limits */
#define SQL_BUFFER_SIZE 2000


struct Options {
    struct Option *input, *dsn, *layer, *type, *format,
	*field, *dsco, *lco, *otype;
};

struct Flags {
    struct Flag *cat, *esristyle, *update, *nocat, *new, *append,
                *force2d, *multi, *list;
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
void list_formats(void);
char *default_driver(void);

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
                 int *, int *, int);
