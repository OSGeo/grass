#include <grass/config.h>
#include <grass/gis.h>
#ifdef HAVE_GDAL
#include <gdal.h>
#endif

#define XDR_FLOAT_NBYTES 4
#define XDR_DOUBLE_NBYTES 8
#define NULL_ROWS_INMEM 8

/* if short is 16 bits, then
 *       short will allow 32767 cols
 *       unsigned short will allow 65536 cols
 * use int if you need more columns (but this will take more memory).
 *
 */
typedef int COLUMN_MAPPING;

struct GDAL_link
{
    char *filename;
    int band_num;
    DCELL null_val;
    int hflip;
    int vflip;
#ifdef HAVE_GDAL
    GDALDatasetH data;
    GDALRasterBandH band;
    GDALDataType type;
#endif
};

#ifdef HAVE_GDAL
extern CPLErr Rast_gdal_raster_IO(GDALRasterBandH, GDALRWFlag,
				  int, int, int, int,
				  void *, int, int, GDALDataType, int, int);
#endif

struct tileinfo		/* Information for tiles */
{
    char *name;			/* Name of open file            */
    char *mapset;		/* Mapset of open file          */
    struct Cell_head cellhd;	/* Cell header                  */
    struct ilist *clist;	/* columns inside current region */
};

struct R_vrt
{
    int tilecount;
    struct tileinfo *tileinfo;
    struct ilist *tlist;
};

struct fileinfo			/* Information for opened cell files */
{
    int open_mode;		/* see defines below            */
    struct Cell_head cellhd;	/* Cell header                  */
    struct Reclass reclass;	/* Table reclass                */
    struct Cell_stats statf;	/* Cell stats                   */
    struct Range range;		/* Range structure              */
    struct FPRange fp_range;	/* float Range structure        */
    int want_histogram;
    int reclass_flag;		/* Automatic reclass flag       */
    off_t *row_ptr;		/* File row addresses           */
    COLUMN_MAPPING *col_map;	/* Data to window col mapping   */
    double C1, C2;		/* Data to window row constants */
    int cur_row;		/* Current data row in memory   */
    int null_cur_row;		/* Current null row in memory   */
    int cur_nbytes;		/* nbytes per cell for current row */
    unsigned char *data;	/* Decompressed data buffer     */
    int null_fd;		/* Null bitmap fd               */
    unsigned char *null_bits;	/* Null bitmap buffer           */
    int nbytes;			/* bytes per cell               */
    RASTER_MAP_TYPE map_type;	/* type: int, float or double map */
    char *temp_name;		/* Temporary name for NEW files */
    char *null_temp_name;	/* Temporary name for NEW NULL files */
    int null_file_exists;	/* for existing raster maps     */
    char *name;			/* Name of open file            */
    char *mapset;		/* Mapset of open file          */
    int io_error;		/* io error warning given       */
    struct Quant quant;
    struct GDAL_link *gdal;
    int data_fd;		/* Raster data fd               */
    off_t *null_row_ptr;	/* Null file row addresses      */
    struct R_vrt *vrt;
};

struct R__			/*  Structure of library globals */
{
    RASTER_MAP_TYPE fp_type;	/* type for writing floating maps */
    int mask_fd;		/* File descriptor for automatic mask   */
    int auto_mask;		/* Flag denoting automatic masking      */
    int want_histogram;
    int nbytes;
    int compression_type;
    int compress_nulls;
    int window_set;		/* Flag: window set?                    */
    int split_window;           /* Separate windows for input and output */
    struct Cell_head rd_window;	/* Window used for input        */
    struct Cell_head wr_window;	/* Window used for output       */

    int fileinfo_count;
    struct fileinfo *fileinfo;
};

extern struct R__ R__;		/* allocated in init */

#define OPEN_OLD              1
#define OPEN_NEW_COMPRESSED   2
#define OPEN_NEW_UNCOMPRESSED 3
