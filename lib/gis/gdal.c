#include <grass/config.h>
#include <grass/gis.h>

#ifdef GDAL_LINK

#include <gdal.h>

struct GDAL_link *G_get_gdal_link(const char *name, const char *mapset)
{
    static int initialized;
    const char *filename;
    int band_num;
    GDALDatasetH data;
    GDALRasterBandH band;
    GDALDataType type;
    struct GDAL_link *gdal;
    RASTER_MAP_TYPE map_type, req_type;
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
	GDALAllRegister();
	initialized = 1;
    }

    data = GDALOpen(filename, GA_ReadOnly);
    if (!data)
	return NULL;

    band = GDALGetRasterBand(data, band_num);
    if (!band) {
	GDALClose(data);
	return NULL;
    }

    gdal = G_calloc(1, sizeof(struct GDAL_link));

    gdal->filename = G_store(filename);
    gdal->band_num = band_num;
    gdal->null_val = null_val;
    gdal->data = data;
    gdal->band = band;
    gdal->type = type;

    return gdal;
}

void G_close_gdal_link(struct GDAL_link *gdal)
{
    GDALClose(gdal->data);
    G_free(gdal->filename);
    G_free(gdal);
}

#endif
