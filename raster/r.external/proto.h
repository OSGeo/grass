#include <grass/raster.h>

#undef MIN
#undef MAX
#define MIN(a,b)      ((a) < (b) ? (a) : (b))
#define MAX(a,b)      ((a) > (b) ? (a) : (b))

struct band_info
{
    RASTER_MAP_TYPE data_type;
    GDALDataType gdal_type;
    int has_null;
    double null_val;
    struct Colors colors;
};

enum flip {
    FLIP_H = 1,
    FLIP_V = 2,
};

/* colors.c */
void transfer_colormap(GDALRasterBandH, const char *);

/* link.c */
void query_band(GDALRasterBandH, const char *,
		struct Cell_head *, struct band_info *);
void make_cell(const char *, const struct band_info *);
void make_link(const char *, const char *, int,
	       const struct band_info *, int);
void write_fp_format(const char *, const struct band_info *);
void write_fp_quant(const char *);
void create_map(const char *, int, const char *,
		struct Cell_head *, struct band_info *,
		const char *, int);

/* list.c */
void list_layers(FILE *, const char *);
void list_formats(void);
void list_bands(struct Cell_head *, GDALDatasetH);

/* proj.c */
void check_projection(struct Cell_head *, GDALDatasetH, char *, int, int, int);

/* window.c */
void setup_window(struct Cell_head *, GDALDatasetH, int *);
void update_default_window(struct Cell_head *);
