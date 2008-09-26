
#include <stdlib.h>
#include <string.h>
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "G.h"

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

static void CPL_STDCALL (*pGDALAllRegister)(void);
static void CPL_STDCALL (*pGDALClose)(GDALDatasetH);
static GDALRasterBandH CPL_STDCALL (*pGDALGetRasterBand)(GDALDatasetH, int);
static GDALDatasetH CPL_STDCALL (*pGDALOpen)(
    const char *pszFilename, GDALAccess eAccess);
static CPLErr CPL_STDCALL (*pGDALRasterIO)(
    GDALRasterBandH hRBand, GDALRWFlag eRWFlag,
    int nDSXOff, int nDSYOff, int nDSXSize, int nDSYSize,
    void * pBuffer, int nBXSize, int nBYSize,GDALDataType eBDataType,
    int nPixelSpace, int nLineSpace);

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
    static const char * const candidates[] = {
# ifdef __unix__
	"libgdal.1.1.so",
	"gdal.1.0.so",
	"gdal.so.1.0",
	"libgdal.so.1",
	"libgdal.so",
# endif
# ifdef _WIN32
	"gdal11.dll",
	"gdal.1.0.dll",
	"gdal.dll",
# endif
	NULL
    };
    int i;

    for (i = 0; candidates[i]; i++) {
	try_load_library(candidates[i]);
	if (library_h)
	    return;
    }

    G_fatal_error(_("Unable to load GDAL library"));
}

static void init_gdal(void)
{
    load_library();

    pGDALAllRegister   = get_symbol("GDALAllRegister");
    pGDALOpen          = get_symbol("GDALOpen");
    pGDALClose         = get_symbol("GDALClose");
    pGDALGetRasterBand = get_symbol("GDALGetRasterBand");
    pGDALRasterIO      = get_symbol("GDALRasterIO");
}

#else /* GDAL_DYNAMIC */

static void init_gdal(void)
{
    pGDALAllRegister   = &GDALAllRegister;
    pGDALOpen          = &GDALOpen;
    pGDALClose         = &GDALClose;
    pGDALGetRasterBand = &GDALGetRasterBand;
    pGDALRasterIO      = &GDALRasterIO;
}

#endif /* GDAL_DYNAMIC */

#endif /* GDAL_LINK */

struct GDAL_link *G_get_gdal_link(const char *name, const char *mapset)
{
#ifdef GDAL_LINK
    static int initialized;
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

    if (!G_find_cell2(name, mapset))
	return NULL;

    map_type = G_raster_map_type(name, mapset);
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
	G_set_d_null_value(&null_val, 1);
    else
	null_val = atof(p);

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

    if (!initialized) {
	init_gdal();
	(*pGDALAllRegister)();
	initialized = 1;
    }

    data = (*pGDALOpen)(filename, GA_ReadOnly);
    if (!data)
	return NULL;

    band = (*pGDALGetRasterBand)(data, band_num);
    if (!band) {
	(*pGDALClose)(data);
	return NULL;
    }
#endif

    gdal = G_calloc(1, sizeof(struct GDAL_link));

    gdal->filename = G_store(filename);
    gdal->band_num = band_num;
    gdal->null_val = null_val;
#ifdef GDAL_LINK
    gdal->data = data;
    gdal->band = band;
    gdal->type = type;
#endif

    return gdal;
}

void G_close_gdal_link(struct GDAL_link *gdal)
{
#ifdef GDAL_LINK
    (*pGDALClose)(gdal->data);
#endif
    G_free(gdal->filename);
    G_free(gdal);
}

#ifdef GDAL_LINK
CPLErr G_gdal_raster_IO(
    GDALRasterBandH band, GDALRWFlag rw_flag,
    int x_off, int y_off, int x_size, int y_size,
    void *buffer, int buf_x_size, int buf_y_size, GDALDataType buf_type,
    int pixel_size, int line_size)
{
    return (*pGDALRasterIO)(
	band, rw_flag, x_off, y_off, x_size, y_size,
	buffer, buf_x_size, buf_y_size, buf_type,
	pixel_size, line_size);
}
#endif
