#include <grass/config.h>
#include <grass/gis.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
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
#ifdef HAVE_GDAL
    GDALDatasetH data;
    GDALRasterBandH band;
    GDALDataType type;
#endif
};

#ifdef HAVE_GDAL
extern CPLErr G_gdal_raster_IO(
    GDALRasterBandH, GDALRWFlag,
    int, int, int, int,
    void *, int, int, GDALDataType,
    int, int);
#endif

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
    int nbytes;			/* bytes per cell               */
    RASTER_MAP_TYPE map_type;	/* type: int, float or double map */
    char *temp_name;		/* Temporary name for NEW files */
    char *null_temp_name;	/* Temporary name for NEW NULL files */
    int null_file_exists;	/* for existing raster maps     */
    char *name;			/* Name of open file            */
    char *mapset;		/* Mapset of open file          */
    int io_error;		/* io error warning given       */
    XDR xdrstream;		/* xdr stream for reading fp    */
    unsigned char *NULL_ROWS[NULL_ROWS_INMEM];
    unsigned char *null_work_buf;	/* data buffer for reading null rows    */
    int min_null_row;		/* Minimum row null row number in memory */
    struct Quant quant;
    struct GDAL_link *gdal;
};

struct G__			/*  Structure of library globals */
{
    int fp_nbytes;		/* size of cell in floating maps */
    RASTER_MAP_TYPE fp_type;	/* type for writing floating maps */
    struct Cell_head window;	/* Contains the current window          */
    int window_set;		/* Flag: window set?                    */
    int mask_fd;		/* File descriptor for automatic mask   */
    int auto_mask;		/* Flag denoting automatic masking      */
    CELL *mask_buf;
    char *null_buf;		/* buffer for reading null rows         */
    CELL *temp_buf;
    unsigned char *compressed_buf;	/* Pre/post compressed data buffer      */
    int compressed_buf_size;	/* sizeof compressed_buf                */
    unsigned char *work_buf;	/* work data buffer                     */
    int work_buf_size;		/* sizeof work_buf                      */
    int null_buf_size;		/* sizeof null_buf                      */
    int mask_buf_size;		/* sizeof mask_buf                      */
    int temp_buf_size;		/* sizeof temp_buf                      */
    int want_histogram;

    int fileinfo_count;
    struct fileinfo *fileinfo;
};

extern struct G__ G__;		/* allocated in gisinit */

#define OPEN_OLD              1
#define OPEN_NEW_COMPRESSED   2
#define OPEN_NEW_UNCOMPRESSED 3
#define OPEN_NEW_RANDOM       4
