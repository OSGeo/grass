/*
 *****************************************************************************
 *
 * MODULE:      Grass Include Files
 * AUTHOR(S):   Original author unknown - probably CERL
 *              Justin Hickey - Thailand - jhickey@hpcc.nectec.or.th
 * PURPOSE:     This file contains definitions of variables and data types
 *              for use with most, if not all, Grass programs. This file is
 *              usually included in every Grass program.
 * COPYRIGHT:   (C) 2000-2011 by the GRASS Development Team
 *
 *              This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/

#ifndef GRASS_GIS_H
#define GRASS_GIS_H

/*============================= Include Files ==============================*/

/* System include files */
#include <stdio.h>
#include <stdarg.h>

/* Grass and local include files */
#include <grass/config.h>
#include <grass/datetime.h>

/*=========================== Constants/Defines ============================*/

#if !defined __GNUC__ || __GNUC__ < 2
#undef __attribute__
#define __attribute__(x)
#endif

static const char *GRASS_copyright __attribute__ ((unused))
    = "GRASS GNU GPL licensed Software";

#define GIS_H_VERSION "$Revision$"
#define GIS_H_DATE    "$Date$"

#define G_gisinit(pgm) G__gisinit(GIS_H_VERSION, (pgm))
#define G_no_gisinit() G__no_gisinit(GIS_H_VERSION)

/* Define TRUE and FALSE for boolean comparisons */
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#if defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64
#define PRI_OFF_T	"lld"
#else
#define PRI_OFF_T	"ld"
#endif

#define NEWLINE     '\n'

/*!
  \brief List of units
*/
#define U_UNDEFINED    -1
#define U_UNKNOWN       0
#define U_ACRES		1
#define U_HECTARES	2
#define U_KILOMETERS	3
#define U_METERS	4
#define U_MILES		5
#define U_FEET		6
#define U_RADIANS	7
#define U_DEGREES	8
/* Temporal units from the datetime library */
#define U_YEARS         DATETIME_YEAR   
#define U_MONTHS        DATETIME_MONTH  
#define U_DAYS          DATETIME_DAY    
#define U_HOURS         DATETIME_HOUR   
#define U_MINUTES       DATETIME_MINUTE 
#define U_SECONDS       DATETIME_SECOND 

/*! \brief Projection code - XY coordinate system (unreferenced data) */
#define PROJECTION_XY     0
/*! \brief Projection code - UTM */
#define PROJECTION_UTM    1
/*! \brief Projection code - State Plane */
#define PROJECTION_SP     2
/*! \brief Projection code - Latitude-Longitude */
#define PROJECTION_LL     3
/*! \brief Projection code - other projection (other then noted above) */
#define PROJECTION_OTHER  99

#define PROJECTION_FILE "PROJ_INFO"
#define UNIT_FILE       "PROJ_UNITS"

#define CONFIG_DIR ".grass7"

/* define PI and friends */
#undef M_PI
#define M_PI    3.14159265358979323846	/* pi */

#undef M_PI_2
#define M_PI_2  1.57079632679489661923	/* pi/2 */

#undef M_PI_4
#define M_PI_4  0.78539816339744830962	/* pi/4 */

/* epsilon (IEEE: 2.220446e-16) */
#define GRASS_EPSILON 1.0e-15

/* Location of envariment variables */
#define G_VAR_GISRC    0
#define G_VAR_MAPSET   1

/* Where to find/store variables */
#define G_GISRC_MODE_FILE     0	/* files */
#define G_GISRC_MODE_MEMORY   1	/* memory only */

/* for G_parser() */
#define TYPE_INTEGER  1
#define TYPE_DOUBLE   2
#define TYPE_STRING   3
#define YES           1
#define NO            0

/* File/directory name lengths */
#define GNAME_MAX 256
#define GMAPSET_MAX 256

#define GPATH_MAX 4096

/* Macros for type size independent integers                    */
/* Use these for portability to ensure integers are truly 32bit */
/* and are handled in a uniform manner                          */

/* Convert integer to 4 bytes - little endian */
#define serialize_int32_le(buf, x) do { \
    (buf)[0] = ((x) >>  0) & 0xFF; \
    (buf)[1] = ((x) >>  8) & 0xFF; \
    (buf)[2] = ((x) >> 16) & 0xFF; \
    (buf)[3] = ((x) >> 24) & 0xFF; \
} while(0)

/* Convert 4 bytes to an integer - little endian */
#define deserialize_int32_le(buf) (((buf)[0] <<  0) | \
                                   ((buf)[1] <<  8) | \
                                   ((buf)[2] << 16) | \
                                   ((buf)[3] << 24))

/* Convert integer to 4 bytes - big endian */
#define serialize_int32_be(buf, x) do { \
    (buf)[0] = ((x) >> 24) & 0xFF; \
    (buf)[1] = ((x) >> 16) & 0xFF; \
    (buf)[2] = ((x) >>  8) & 0xFF; \
    (buf)[3] = ((x) >>  0) & 0xFF; \
} while(0)

/* Convert 4 bytes to an integer - big endian */
#define deserialize_int32_be(buf) (((buf)[0] << 24) | \
                                   ((buf)[1] << 16) | \
                                   ((buf)[2] <<  8) | \
                                   ((buf)[3] <<  0))

/* Cross-platform Directory Separator Character and null device stuff */
#define GRASS_DIRSEP '/'
#ifdef __MINGW32__
#  define HOST_DIRSEP '\\'
#  define G_DEV_NULL "NUL:"
#else
#  define HOST_DIRSEP '/'
#  define G_DEV_NULL "/dev/null"
#endif

/**/ typedef enum
{
    G_OPT_UNDEFINED,
    G_OPT_DB_SQL,		/* SQL statements */
    G_OPT_DB_WHERE,		/* SQL where conditions */
    G_OPT_DB_TABLE,		/* table name */
    G_OPT_DB_DRIVER,		/* driver name */
    G_OPT_DB_DATABASE,		/* database name */
    G_OPT_DB_SCHEMA,            /* database schema */
    G_OPT_DB_COLUMN,		/* one attr column */
    G_OPT_DB_COLUMNS,		/* one or more attr columns */
    G_OPT_DB_KEYCOLUMN,         /* key column */

    G_OPT_I_GROUP,		/* old input imagery group */
    G_OPT_I_SUBGROUP,		/* old input imagery subgroup */
    G_OPT_R_INPUT,		/* old input raster map */
    G_OPT_R_INPUTS,		/* old input raster maps */
    G_OPT_R_OUTPUT,		/* new output raster map */
    G_OPT_R_MAP,		/* old input raster map */
    G_OPT_R_MAPS,		/* old input rasters map */
    G_OPT_R_BASE,		/* old input base raster map */
    G_OPT_R_COVER,		/* old input cover raster map */
    G_OPT_R_ELEV,		/* old input elevation raster map */
    G_OPT_R_ELEVS,		/* old input elevation raster maps */
    G_OPT_R_INTERP_TYPE,        /* interpolation type */

    G_OPT_R3_INPUT,		/* old input raster3d map */
    G_OPT_R3_INPUTS,		/* old input raster3d maps */
    G_OPT_R3_OUTPUT,		/* new output raster3d map */
    G_OPT_R3_MAP,		/* old input raster3d map */
    G_OPT_R3_MAPS,		/* old input raster3d maps */
    G_OPT_R3_TYPE,              /* Type (FCELL or DCELL) of a new created raster3d map */
    G_OPT_R3_PRECISION,         /* The precision of the new generated raster3d map */
    G_OPT_R3_TILE_DIMENSION,    /* The tile dimension of a new generated raster3d map */
    G_OPT_R3_COMPRESSION,       /* The kind of compression of a new created raster3d map */

    G_OPT_V_INPUT,		/* old input vector map */
    G_OPT_V_INPUTS,		/* old input vector maps */
    G_OPT_V_OUTPUT,		/* new output vector map */
    G_OPT_V_MAP,		/* old input vector map */
    G_OPT_V_MAPS,		/* old input vector maps */
    G_OPT_V_TYPE,		/* primitive type */
    G_OPT_V3_TYPE,		/* primitive type, 2D and 3D */
    G_OPT_V_FIELD,		/* layer number (layers used to be called fields) */
    G_OPT_V_FIELD_ALL,		/* layer number (layers used to be called fields) */
    G_OPT_V_CAT,		/* one category */
    G_OPT_V_CATS,		/* more categories */
    G_OPT_V_ID, 		/* one feature id */
    G_OPT_V_IDS,		/* more feature ids */

    G_OPT_F_INPUT,		/* old input file */
    G_OPT_F_OUTPUT,		/* new output file */
    G_OPT_F_SEP,		/* data field separator */

    G_OPT_C_FG,			/* foreground color */
    G_OPT_C_BG,			/* background color */

    G_OPT_M_UNITS,              /* units */
    G_OPT_M_DATATYPE,           /* datatype */
    G_OPT_M_MAPSET,             /* mapset */
    G_OPT_M_COORDS,             /* coordinates */
    G_OPT_M_COLR,               /* color rules */
    G_OPT_M_DIR,                /* directory input */    

    G_OPT_STDS_INPUT,           /* old input space time dataset of type strds, str3ds or stvds */
    G_OPT_STDS_INPUTS,          /* old input space time datasets */
    G_OPT_STDS_OUTPUT,          /* new output space time dataset */
    G_OPT_STRDS_INPUT,          /* old input space time raster dataset */
    G_OPT_STRDS_INPUTS,         /* old input space time raster datasets */
    G_OPT_STRDS_OUTPUT,         /* new output space time raster dataset */
    G_OPT_STR3DS_INPUT,         /* old input space time raster3d dataset */
    G_OPT_STR3DS_INPUTS,        /* old input space time raster3d datasets */
    G_OPT_STR3DS_OUTPUT,        /* new output space time raster3d dataset */
    G_OPT_STVDS_INPUT,          /* old input space time vector dataset */
    G_OPT_STVDS_INPUTS,         /* old input space time vector datasets */
    G_OPT_STVDS_OUTPUT,         /* new output space time vector dataset */
    G_OPT_MAP_INPUT,            /* old input map of type raster, vector or raster3d  */
    G_OPT_MAP_INPUTS,           /* old input maps of type raster, vector or raster3d  */
    G_OPT_STDS_TYPE,            /* the type of a space time dataset: strds, str3ds, stvds */ 
    G_OPT_MAP_TYPE,             /* The type of an input map: raster, vect, rast3d */
    G_OPT_T_TYPE,               /* The temporal type of a space time dataset */
    G_OPT_T_WHERE,              /* A temporal GIS framework SQL WHERE statement */
    G_OPT_T_SAMPLE,             /* Temporal sample methods */

} STD_OPT;

/**/ typedef enum
{
    G_FLG_UNDEFINED,
    G_FLG_V_TABLE,		/* do not create attribute table */
    G_FLG_V_TOPO,               /* do not build topology */
} STD_FLG;

/* Message format */
#define G_INFO_FORMAT_STANDARD 0	/* GRASS_MESSAGE_FORMAT=standard or not defined */
#define G_INFO_FORMAT_GUI      1	/* GRASS_MESSAGE_FORMAT=gui */
#define G_INFO_FORMAT_SILENT   2	/* GRASS_MESSAGE_FORMAT=silent */
#define G_INFO_FORMAT_PLAIN    3	/* GRASS_MESSAGE_FORMAT=plain */

/* Icon types */
#define G_ICON_CROSS  0
#define G_ICON_BOX    1
#define G_ICON_ARROW  2

/* default colors */
#define DEFAULT_FG_COLOR "black"
#define DEFAULT_BG_COLOR "white"

/* error codes */
#define G_FATAL_EXIT    0
#define G_FATAL_PRINT   1
#define G_FATAL_RETURN  2

/*! \brief Endian check */
#define ENDIAN_LITTLE 0
#define ENDIAN_BIG    1
#define ENDIAN_OTHER  2

/* for vector maps */
/*!
  \brief Name of default key column
*/
#define GV_KEY_COLUMN    "cat"

/* Element types */
enum
{				/* Dir */
    G_ELEMENT_RASTER = 1,	/* cell */
    G_ELEMENT_RASTER3D = 2,	/* 3dcell */
    G_ELEMENT_VECTOR = 3,	/* vector */
    G_ELEMENT_OLDVECTOR = 4,	/* GRASS < 5.7 vector */
    G_ELEMENT_ASCIIVECTOR = 5,	/* ASCII vector */
    G_ELEMENT_ICON = 6,		/* icon */
    G_ELEMENT_LABEL = 7,	/* labels */
    G_ELEMENT_SITE = 8,		/* sites */
    G_ELEMENT_REGION = 9,	/* region */
    G_ELEMENT_REGION3D = 10,	/* 3dregion */
    G_ELEMENT_GROUP = 11,	/* group */
    G_ELEMENT_3DVIEW = 12	/* 3dview */
};

/*=========================== Typedefs/Structures ==========================*/

/*!
  \brief 2D/3D raster map header (used also for region)
*/
struct Cell_head
{
    /*! \brief Max number of bytes per raster data value minus 1 (raster header only)

     Note: -1 for FP raster maps
    */
    int format;
    /*! \brief Compression mode (raster header only)

      - 0: uncompressed
      - 1: compressed
      - -1: pre GRASS 3.0
    */
    int compressed;
    /*! \brief Number of rows for 2D data */
    int rows;
    /*! \brief Number of rows for 3D data */
    int rows3;
    /*! \brief Number of columns for 2D data */
    int cols;
    /*! \brief Number of columns for 3D data */
    int cols3;
    /*! \brief number of depths for 3D data */
    int depths;
    /*! \brief Projection code

      - PROJECTION_XY
      - PROJECTION_UTM
      - PROJECTION_SP
      - PROJECTION_LL
      - PROJECTION_OTHER
     */
    int proj;
    /*! \brief Projection zone (UTM) */
    int zone;
    /*! \brief Resolution - east to west cell size for 2D data */
    double ew_res;
    /*! \brief Resolution - east to west cell size for 3D data */
    double ew_res3;   
    /*! \brief Resolution - north to south cell size for 2D data */
    double ns_res;     
    /*! \brief Resolution - north to south cell size for 3D data */
    double ns_res3;   
    /*! \brief Resolution - top to bottom cell size for 3D data */
    double tb_res;    
    /*! \brief Extent coordinates (north) */
    double north;     
    /*! \brief Extent coordinates (south) */
    double south;
    /*! \brief Extent coordinates (east) */
    double east;
    /*! \brief Extent coordinates (west) */
    double west;
    /*! \brief Extent coordinates (top) - 3D data*/
    double top;
    /*! \brief Extent coordinates (bottom) - 3D data */
    double bottom;
};

/*
 ** Structure for I/O of 3dview files  (view.c)
 */
struct G_3dview
{
    char pgm_id[40];		/* user-provided identifier */
    float from_to[2][3];	/* eye position & lookat position */
    float fov;			/* field of view */
    float twist;		/* right_hand rotation about from_to */
    float exag;			/* terrain elevation exageration */
    int mesh_freq;		/* cells per grid line */
    int poly_freq;		/* cells per polygon */
    int display_type;		/* 1 for mesh, 2 for poly, 3 for both */
    int lightson;		/* boolean */
    int dozero;			/* boolean */
    int colorgrid;		/* boolean */
    int shading;		/* boolean */
    int fringe;			/* boolean */
    int surfonly;		/* boolean */
    int doavg;			/* boolean */
    char grid_col[40];		/* colors */
    char bg_col[40];		/* colors */
    char other_col[40];		/* colors */
    float lightpos[4];		/* east, north, height, 1.0 for local 0.0 infin */
    float lightcol[3];		/* values between 0.0 to 1.0 for red, grn, blu */
    float ambient;
    float shine;
    struct Cell_head vwin;
};

struct Key_Value
{
    int nitems;
    int nalloc;
    char **key;
    char **value;
};

struct Option			/* Structure that stores option info */
{
    const char *key;		/* Key word used on command line    */
    int type;			/* Option type                      */
    int required;		/* REQUIRED or OPTIONAL             */
    int multiple;		/* Multiple entries OK              */
    const char *options;	/* Approved values or range or NULL */
    const char **opts;		/* NULL or NULL terminated array of parsed options */
    const char *key_desc;	/* one word describing the key      */
    const char *label;		/* Optional short label, used in GUI as item label */
    const char *description;	/* String describing option         */
    const char *descriptions;	/* ';' separated pairs of option and option descriptions */
    /* For example: (with ->options = "break,rmdupl")
     * "break;break lines on intersections;"
     * "rmdupl;remove duplicates"
     */
    const char **descs;		/* parsed descriptions, array of either NULL or string */
    /* in the same order as options */
    char *answer;		/* Option answer                    */
    const char *def;		/* Where original answer gets saved */
    char **answers;		/* Option answers (for multiple=YES) */
    struct Option *next_opt;	/* Pointer to next option struct    */
    const char *gisprompt;	/* Interactive prompt guidance      */
    const char *guisection;	/* GUI Layout guidance: ';' delimited heirarchical tree position */
    const char *guidependency;  /* GUI dependency, list of options
				   (separated by commas) to be updated
				   if the value is chanched */
    int (*checker)(const char *);/* Routine to check answer or NULL  */
    int count;
};

struct Flag			/* Structure that stores flag info  */
{
    char key;			/* Key char used on command line    */
    char answer;		/* Stores flag state: 0/1           */
    char suppress_required;	/* Suppresses checking of required options */
    const char *label;		/* Optional short label, used in GUI as item label */
    const char *description;	/* String describing flag meaning   */
    const char *guisection;	/* GUI Layout guidance: ';' delimited heirarchical tree position */
    struct Flag *next_flag;	/* Pointer to next flag struct      */
};

struct GModule			/* Structure that stores module info  */
{
    const char *label;		/* Optional short description for GUI */
    const char *description;	/* String describing module */
    const char **keywords;	/* Keywords describing module */
    /* further items are possible: author(s), version */
    int overwrite;		/* overwrite old files */
    int verbose;		/* print all information about progress and so on */
};

struct TimeStamp
{
    DateTime dt[2];		/* two datetimes */
    int count;
};

struct Counter {
    int value;
};

struct Popen {
    FILE *fp;
    int pid;
};

typedef int CELL;
typedef double DCELL;
typedef float FCELL;

struct _Color_Value_
{
    DCELL value;
    unsigned char red;
    unsigned char grn;
    unsigned char blu;
};

struct _Color_Rule_
{
    struct _Color_Value_ low, high;
    struct _Color_Rule_ *next;
    struct _Color_Rule_ *prev;
};

struct _Color_Info_
{
    struct _Color_Rule_ *rules;
    int n_rules;

    struct
    {
	unsigned char *red;
	unsigned char *grn;
	unsigned char *blu;
	unsigned char *set;
	int nalloc;
	int active;
    } lookup;

    struct
    {
	DCELL *vals;
	/* pointers to color rules corresponding to the intervals btwn vals */
	struct _Color_Rule_ **rules;
	int nalloc;
	int active;
    } fp_lookup;

    DCELL min, max;
};

struct Colors
{
    int version;		/* set by read_colors: -1=old,1=new */
    DCELL shift;
    int invert;
    int is_float;		/* defined on floating point raster data? */
    int null_set;		/* the colors for null are set? */
    unsigned char null_red;
    unsigned char null_grn;
    unsigned char null_blu;
    int undef_set;		/* the colors for cells not in range are set? */
    unsigned char undef_red;
    unsigned char undef_grn;
    unsigned char undef_blu;
    struct _Color_Info_ fixed;
    struct _Color_Info_ modular;
    DCELL cmin;
    DCELL cmax;
    int organizing;
};

/*!
  \brief List of integers
*/
struct ilist
{
    /*!
      \brief Array of values
    */
    int *value;
    /*!
      \brief Number of values in the list
    */
    int n_values;
    /*!
      \brief Allocated space for values
    */
    int alloc_values;
};

/*============================== Prototypes ================================*/

/* Since there are so many prototypes for the gis library they are stored */
/* in the file gisdefs.h */
#include <grass/defs/gis.h>

#endif /* GRASS_GIS_H */
