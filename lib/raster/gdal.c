/*!
  \file lib/raster/gdal.c
  
  \brief Raster Library - Utilization of GDAL library.
  
  (C) 2010 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Glynn Clements
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <grass/config.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "R.h"

#ifndef HAVE_GDAL
#undef GDAL_LINK
#endif

#ifdef GDAL_LINK

#ifdef GDAL_DYNAMIC
# if defined(__unix) || defined(__unix__)
#  include <dlfcn.h>
# endif
# ifdef _WIN32
#  include <windows.h>
# endif
#endif

static void CPL_STDCALL(*pGDALAllRegister) (void);
static void CPL_STDCALL(*pGDALClose) (GDALDatasetH);
static GDALRasterBandH CPL_STDCALL(*pGDALGetRasterBand) (GDALDatasetH, int);
static GDALDatasetH CPL_STDCALL(*pGDALOpen) (const char *pszFilename,
					     GDALAccess eAccess);
static CPLErr CPL_STDCALL(*pGDALRasterIO) (GDALRasterBandH hRBand,
					   GDALRWFlag eRWFlag, int nDSXOff,
					   int nDSYOff, int nDSXSize,
					   int nDSYSize, void *pBuffer,
					   int nBXSize, int nBYSize,
					   GDALDataType eBDataType,
					   int nPixelSpace, int nLineSpace);
static GDALDriverH CPL_STDCALL(*pGDALGetDriverByName) (const char *);
static const char *CPL_STDCALL(*pGDALGetMetadataItem) (GDALMajorObjectH,
						       const char *,
						       const char *);
static GDALDatasetH CPL_STDCALL(*pGDALCreate) (GDALDriverH hDriver,
					       const char *, int, int, int,
					       GDALDataType, char **);
static GDALDatasetH CPL_STDCALL(*pGDALCreateCopy) (GDALDriverH, const char *,
						   GDALDatasetH, int, char **,
						   GDALProgressFunc, void *);
static CPLErr CPL_STDCALL(*pGDALSetRasterNoDataValue) (GDALRasterBandH,
						       double);
static CPLErr CPL_STDCALL(*pGDALSetGeoTransform) (GDALDatasetH, double *);
static CPLErr CPL_STDCALL(*pGDALSetProjection) (GDALDatasetH, const char *);
static const char *CPL_STDCALL(*pGDALGetDriverShortName) (GDALDriverH);
static GDALDriverH CPL_STDCALL(*pGDALGetDatasetDriver) (GDALDatasetH);

#if GDAL_DYNAMIC
# if defined(__unix) && !defined(__unix__)
#  define __unix__ __unix
# endif

static void *library_h;

static void *get_symbol(const char *name)
{
    void *sym;

# ifdef __unix__
    sym = dlsym(library_h, name);
# endif
# ifdef _WIN32
    sym = GetProcAddress((HINSTANCE) library_h, name);
# endif

    if (!sym)
	G_fatal_error(_("Unable to locate symbol <%s>"), name);

    return sym;
}

static void try_load_library(const char *name)
{
# ifdef __unix__
    library_h = dlopen(name, RTLD_NOW);
# endif
# ifdef _WIN32
    library_h = LoadLibrary(name);
# endif
}

static void load_library(void)
{
    static const char *const candidates[] = {
# ifdef __unix__
	"libgdal.1.1.so",
	"libgdal1.6.0.so",
	"gdal.1.0.so",
	"gdal.so.1.0",
	"libgdal.so.1",
	"libgdal.so",
	"libgdal1.6.0.so",
	"libgdal1.7.0.so",
# endif
# ifdef _WIN32
	"gdal110.dll",
	"gdal19.dll",
	"gdal18.dll",
	"gdal17.dll",
	"gdal16.dll",
	"gdal15.dll",
	"gdal11.dll",
	"gdal.1.0.dll",
	"libgdal-1.dll",
	"gdal.dll",
# endif
	NULL
    };
    int i;

    for (i = 0; candidates[i]; i++) {
	try_load_library(candidates[i]);
	if (library_h) {
	    G_debug(3, "found %s", candidates[i]);
	    return;
	}
    }

    G_fatal_error(_("Unable to load GDAL library"));
}

static void init_gdal(void)
{
    load_library();

# if defined(_WIN32) && !defined(_WIN64)
    pGDALAllRegister = get_symbol("_GDALAllRegister@0");
    pGDALOpen = get_symbol("_GDALOpen@8");
    pGDALClose = get_symbol("_GDALClose@4");
    pGDALGetRasterBand = get_symbol("_GDALGetRasterBand@8");
    pGDALRasterIO = get_symbol("_GDALRasterIO@48");
    pGDALGetDriverByName = get_symbol("_GDALGetDriverByName@4");
    pGDALGetMetadataItem = get_symbol("_GDALGetMetadataItem@12");
    pGDALCreate = get_symbol("_GDALCreate@28");
    pGDALCreateCopy = get_symbol("_GDALCreateCopy@28");
    pGDALSetRasterNoDataValue = get_symbol("_GDALSetRasterNoDataValue@12");
    pGDALSetGeoTransform = get_symbol("_GDALSetGeoTransform@8");
    pGDALSetProjection = get_symbol("_GDALSetProjection@8");
    pGDALGetDriverShortName = get_symbol("_GDALGetDriverShortName@4");
    pGDALGetDatasetDriver = get_symbol("_GDALGetDatasetDriver@4");
#else
    pGDALAllRegister = get_symbol("GDALAllRegister");
    pGDALOpen = get_symbol("GDALOpen");
    pGDALClose = get_symbol("GDALClose");
    pGDALGetRasterBand = get_symbol("GDALGetRasterBand");
    pGDALRasterIO = get_symbol("GDALRasterIO");
    pGDALGetDriverByName = get_symbol("GDALGetDriverByName");
    pGDALGetMetadataItem = get_symbol("GDALGetMetadataItem");
    pGDALCreate = get_symbol("GDALCreate");
    pGDALCreateCopy = get_symbol("GDALCreateCopy");
    pGDALSetRasterNoDataValue = get_symbol("GDALSetRasterNoDataValue");
    pGDALSetGeoTransform = get_symbol("GDALSetGeoTransform");
    pGDALSetProjection = get_symbol("GDALSetProjection");
    pGDALGetDriverShortName = get_symbol("GDALGetDriverShortName");
    pGDALGetDatasetDriver = get_symbol("GDALGetDatasetDriver");
#endif
}

#else /* GDAL_DYNAMIC */

static void init_gdal(void)
{
    pGDALAllRegister = &GDALAllRegister;
    pGDALOpen = &GDALOpen;
    pGDALClose = &GDALClose;
    pGDALGetRasterBand = &GDALGetRasterBand;
    pGDALRasterIO = &GDALRasterIO;
    pGDALGetDriverByName = &GDALGetDriverByName;
    pGDALGetMetadataItem = &GDALGetMetadataItem;
    pGDALCreate = &GDALCreate;
    pGDALCreateCopy = &GDALCreateCopy;
    pGDALSetRasterNoDataValue = &GDALSetRasterNoDataValue;
    pGDALSetGeoTransform = &GDALSetGeoTransform;
    pGDALSetProjection = &GDALSetProjection;
    pGDALGetDriverShortName = &GDALGetDriverShortName;
    pGDALGetDatasetDriver = &GDALGetDatasetDriver;
}

#endif /* GDAL_DYNAMIC */

#endif /* GDAL_LINK */

/*!
  \brief Initialization

  Register all GDAL drivers.
*/
void Rast_init_gdal(void)
{
#ifdef GDAL_LINK
    static int initialized;

    if (G_is_initialized(&initialized))
	return;

    init_gdal();
    (*pGDALAllRegister) ();
    G_initialize_done(&initialized);
#endif
}

/*!
  \brief Get GDAL link settings for given raster map

  \param name map name
  \param mapset name of mapset

  \return pointer to GDAL_link structure
  \return NULL if link not found
*/
struct GDAL_link *Rast_get_gdal_link(const char *name, const char *mapset)
{
#ifdef GDAL_LINK
    GDALDatasetH data;
    GDALRasterBandH band;
    GDALDataType type;
    RASTER_MAP_TYPE req_type;
#endif
    const char *filename;
    int band_num;
    struct GDAL_link *gdal;
    RASTER_MAP_TYPE map_type;
    FILE *fp;
    struct Key_Value *key_val;
    const char *p;
    DCELL null_val;
    int hflip, vflip;

    if (!G_find_raster2(name, mapset))
	return NULL;

    map_type = Rast_map_type(name, mapset);
    if (map_type < 0)
	return NULL;

    fp = G_fopen_old_misc("cell_misc", "gdal", name, mapset);
    if (!fp)
	return NULL;
    key_val = G_fread_key_value(fp);
    fclose(fp);

    if (!key_val)
	return NULL;

    filename = G_find_key_value("file", key_val);
    if (!filename)
	return NULL;

    p = G_find_key_value("band", key_val);
    if (!p)
	return NULL;
    band_num = atoi(p);
    if (!band_num)
	return NULL;

    p = G_find_key_value("null", key_val);
    if (!p)
	return NULL;
    if (strcmp(p, "none") == 0)
	Rast_set_d_null_value(&null_val, 1);
    else
	null_val = atof(p);

    hflip = G_find_key_value("hflip", key_val) ? 1 : 0;
    vflip = G_find_key_value("vflip", key_val) ? 1 : 0;

#ifdef GDAL_LINK
    p = G_find_key_value("type", key_val);
    if (!p)
	return NULL;
    type = atoi(p);

    switch (type) {
    case GDT_Byte:
    case GDT_Int16:
    case GDT_UInt16:
    case GDT_Int32:
    case GDT_UInt32:
	req_type = CELL_TYPE;
	break;
    case GDT_Float32:
	req_type = FCELL_TYPE;
	break;
    case GDT_Float64:
	req_type = DCELL_TYPE;
	break;
    default:
	return NULL;
    }

    if (req_type != map_type)
	return NULL;

    Rast_init_gdal();

    data = (*pGDALOpen) (filename, GA_ReadOnly);
    if (!data)
	return NULL;

    band = (*pGDALGetRasterBand) (data, band_num);
    if (!band) {
	(*pGDALClose) (data);
	return NULL;
    }
#endif

    gdal = G_calloc(1, sizeof(struct GDAL_link));

    gdal->filename = G_store(filename);
    gdal->band_num = band_num;
    gdal->null_val = null_val;
    gdal->hflip = hflip;
    gdal->vflip = vflip;
#ifdef GDAL_LINK
    gdal->data = data;
    gdal->band = band;
    gdal->type = type;
#endif

    return gdal;
}

struct GDAL_Options
{
    const char *dir;
    const char *ext;
    const char *format;
    char **options;
};

static struct state
{
    int initialized;
    struct GDAL_Options opts;
    struct Key_Value *projinfo, *projunits;
    char *srswkt;
} state;

static struct state *st = &state;

static void read_gdal_options(void)
{
    FILE *fp;
    struct Key_Value *key_val;
    const char *p;

    fp = G_fopen_old("", "GDAL", G_mapset());
    if (!fp)
	G_fatal_error(_("Unable to open GDAL file"));
    key_val = G_fread_key_value(fp);
    fclose(fp);

    p = G_find_key_value("directory", key_val);
    if (!p)
	p = "gdal";
    if (*p == '/') {
	st->opts.dir = G_store(p);
    }
    else {
	char path[GPATH_MAX];

	G_file_name(path, p, "", G_mapset());
	st->opts.dir = G_store(path);
	if (access(path, 0) != 0)
	    G__make_mapset_element(p);
    }

    p = G_find_key_value("extension", key_val);
    st->opts.ext = G_store(p ? p : "");

    p = G_find_key_value("format", key_val);
    st->opts.format = G_store(p ? p : "GTiff");

    p = G_find_key_value("options", key_val);
    st->opts.options = p ? G_tokenize(p, ",") : NULL;

    G_free_key_value(key_val);
}

/*!
  \brief Create GDAL settings for given raster map

  \param name map name
  \param map_type map type (CELL, FCELL, DCELL)

  \return pointer to allocated GDAL_link structure
  \return NULL on error
*/
struct GDAL_link *Rast_create_gdal_link(const char *name,
					RASTER_MAP_TYPE map_type)
{
#ifdef GDAL_LINK
    char path[GPATH_MAX];
    GDALDriverH driver;
    double transform[6];
    struct GDAL_link *gdal;
    FILE *fp;
    struct Key_Value *key_val;
    char buf[32];

    Rast__init_window();

    Rast_init_gdal();

    if (!G_is_initialized(&st->initialized)) {
	read_gdal_options();
	st->projinfo = G_get_projinfo();
	st->projunits = G_get_projunits();
#if 0
	/* We cannot use GPJ_grass_to_wkt() here because that would create a
	   circular dependency between libgis and libgproj */
	if (st->projinfo && st->projunits)
	    st->srswkt = GPJ_grass_to_wkt(st->projinfo, st->projunits);
#endif
	G_initialize_done(&st->initialized);
    }

    gdal = G_calloc(1, sizeof(struct GDAL_link));

    sprintf(path, "%s/%s%s", st->opts.dir, name, st->opts.ext);
    gdal->filename = G_store(path);
    gdal->band_num = 1;
    gdal->hflip = 0;
    gdal->vflip = 0;

    switch (map_type) {
    case CELL_TYPE:
	switch (R__.nbytes) {
	case 1:
	    gdal->type = GDT_Byte;
	    gdal->null_val = (DCELL) 0xFF;
	    break;
	case 2:
	    gdal->type = GDT_UInt16;
	    gdal->null_val = (DCELL) 0xFFFF;
	    break;
	case 3:
	case 4:
	    gdal->type = GDT_Int32;
	    gdal->null_val = (DCELL) 0x80000000U;
	    break;
	}
	break;
    case FCELL_TYPE:
	gdal->type = GDT_Float32;
	Rast_set_d_null_value(&gdal->null_val, 1);
	break;
    case DCELL_TYPE:
	gdal->type = GDT_Float64;
	Rast_set_d_null_value(&gdal->null_val, 1);
	break;
    default:
	G_fatal_error(_("Invalid map type <%d>"), map_type);
	break;
    }

    driver = (*pGDALGetDriverByName) (st->opts.format);
    if (!driver)
	G_fatal_error(_("Unable to get <%s> driver"), st->opts.format);

    /* Does driver support GDALCreate ? */
    if ((*pGDALGetMetadataItem) (driver, GDAL_DCAP_CREATE, NULL)) {
	gdal->data =
	    (*pGDALCreate)(driver, gdal->filename,
			   R__.wr_window.cols, R__.wr_window.rows,
			   1, gdal->type, st->opts.options);
	if (!gdal->data)
	    G_fatal_error(_("Unable to create <%s> dataset using <%s> driver"),
			  name, st->opts.format);
    }
    /* If not - create MEM driver for intermediate dataset. 
     * Check if raster can be created at all (with GDALCreateCopy) */
    else if ((*pGDALGetMetadataItem) (driver, GDAL_DCAP_CREATECOPY, NULL)) {
	GDALDriverH mem_driver;

	G_message(_("Driver <%s> does not support direct writing. "
		    "Using MEM driver for intermediate dataset."),
		  st->opts.format);

	mem_driver = (*pGDALGetDriverByName) ("MEM");
	if (!mem_driver)
	    G_fatal_error(_("Unable to get in-memory raster driver"));

	gdal->data =
	    (*pGDALCreate)(mem_driver, "",
			   R__.wr_window.cols, R__.wr_window.rows,
			   1, gdal->type, st->opts.options);
	if (!gdal->data)
	    G_fatal_error(_("Unable to create <%s> dataset using memory driver"),
			  name);
    }
    else
	G_fatal_error(_("Driver <%s> does not support creating rasters"),
		      st->opts.format);

    gdal->band = (*pGDALGetRasterBand) (gdal->data, gdal->band_num);

    (*pGDALSetRasterNoDataValue) (gdal->band, gdal->null_val);

    /* Set Geo Transform  */
    transform[0] = R__.wr_window.west;
    transform[1] = R__.wr_window.ew_res;
    transform[2] = 0.0;
    transform[3] = R__.wr_window.north;
    transform[4] = 0.0;
    transform[5] = -R__.wr_window.ns_res;

    if ((*pGDALSetGeoTransform) (gdal->data, transform) >= CE_Failure)
	G_warning(_("Unable to set geo transform"));

    if (st->srswkt)
	if ((*pGDALSetProjection) (gdal->data, st->srswkt) == CE_Failure)
	    G_warning(_("Unable to set projection"));

    fp = G_fopen_new_misc("cell_misc", "gdal", name);
    if (!fp)
	G_fatal_error(_("Unable to create cell_misc/%s/gdal file"), name);

    key_val = G_create_key_value();

    G_set_key_value("file", gdal->filename, key_val);

    sprintf(buf, "%d", gdal->band_num);
    G_set_key_value("band", buf, key_val);

    sprintf(buf, "%.22g", gdal->null_val);
    G_set_key_value("null", buf, key_val);

    sprintf(buf, "%d", gdal->type);
    G_set_key_value("type", buf, key_val);

    if (G_fwrite_key_value(fp, key_val) < 0)
	G_fatal_error(_("Error writing cell_misc/%s/gdal file"), name);

    G_free_key_value(key_val);

    fclose(fp);

    return gdal;
#else
    return NULL;
#endif
}

/*!
  \brief Close existing GDAL link
  
  \param gdal pointer to GDAL_link to be closed
*/
void Rast_close_gdal_link(struct GDAL_link *gdal)
{
#ifdef GDAL_LINK
    (*pGDALClose) (gdal->data);
#endif
    G_free(gdal->filename);
    G_free(gdal);
}

/*!
  \brief Close exising GDAL link and write out data

  \param gdal pointer to GDAL_link to be closed

  \return 1 on success
  \return -1 on failure
*/
int Rast_close_gdal_write_link(struct GDAL_link *gdal)
{
    int stat = 1;

#ifdef GDAL_LINK
    GDALDriverH src_drv = (*pGDALGetDatasetDriver) (gdal->data);

    if (G_strcasecmp((*pGDALGetDriverShortName) (src_drv), "MEM") == 0) {
	GDALDriverH dst_drv = (*pGDALGetDriverByName) (st->opts.format);
	GDALDatasetH dst =
	    (*pGDALCreateCopy) (dst_drv, gdal->filename, gdal->data, FALSE,
				st->opts.options, NULL, NULL);

	if (!dst) {
	    G_warning(_("Unable to create output file <%s> using driver <%s>"),
		      gdal->filename, st->opts.format);
	    stat = -1;
	}
	(*pGDALClose) (dst);
    }

    (*pGDALClose) (gdal->data);

#endif
    G_free(gdal->filename);
    G_free(gdal);

    return stat;
}

#ifdef GDAL_LINK
/*!
  \brief Input/output function for GDAL links

  See GDAL's RasterIO for details.
*/
CPLErr Rast_gdal_raster_IO(GDALRasterBandH band, GDALRWFlag rw_flag,
			   int x_off, int y_off, int x_size, int y_size,
			   void *buffer, int buf_x_size, int buf_y_size,
			   GDALDataType buf_type, int pixel_size,
			   int line_size)
{
    return (*pGDALRasterIO) (band, rw_flag, x_off, y_off, x_size, y_size,
			     buffer, buf_x_size, buf_y_size, buf_type,
			     pixel_size, line_size);
}
#endif
