#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/segment.h>

#define NOT_IN_REGION(x) (r + nextr[(x)] < 0 || r + nextr[(x)] > (nrows - 1) || \
                          c + nextc[(x)] < 0 || c + nextc[(x)] > (ncols - 1))
#define NR(x) (r + nextr[(x)])
#define NC(x) (c + nextc[(x)])
#define INDEX(r,c) ((r) * ncols + (c))
#define DIAG(x) (((x) + 4) > 8 ? ((x) - 4) : ((x) + 4))

#define SROWS 256
#define SCOLS 256

typedef struct {
	void **map; /* matrix of data */
	double min, max; /* data range : may requre casting */
	int nrows, ncols;
	char *map_name; /* map name, unused */
	RASTER_MAP_TYPE data_type; /* type of data */
	size_t data_size; /* type of data */
} MAP;

typedef struct {
	SEGMENT seg;		/* segmented data store */
	int fd;					/* segment temporary file name descriptor */
	char *filename; /* segment temporary file name */
	char *map_name; /* map name converted to segment */
	char *mapset;
	int nrows, ncols; /* store nrows and rcols */
	RASTER_MAP_TYPE data_type; /* data type of the map */
	size_t data_size; /* size of cell returned by sizeof */
	double min, max; /* data range */
} SEG;


/* all in ram functions */
int ram_create_map(MAP *, RASTER_MAP_TYPE);
int ram_read_map(MAP *, char *, int, RASTER_MAP_TYPE);
int ram_reset_map(MAP *, int);
int ram_write_map(MAP *, char *, RASTER_MAP_TYPE, int, double);
int ram_release_map(MAP *);
int ram_destory_map(MAP *);

/* memory swap functions */
int seg_create_map(SEG *, int, int, int, RASTER_MAP_TYPE);
int seg_read_map(SEG *, char *, int, RASTER_MAP_TYPE);
int seg_reset_map (SEG *, int);
int seg_write_map(SEG *, char *, RASTER_MAP_TYPE, int, double);
int seg_release_map(SEG *);
