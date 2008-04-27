/*
* $Id$
*
*****************************************************************************
*
* MODULE:   	Grass Include Files
* AUTHOR(S):	Original author unknown - probably CERL
*   	    	Justin Hickey - Thailand - jhickey@hpcc.nectec.or.th
* PURPOSE:  	This file contains definitions of variables and data types
*   	    	for use with most, if not all, Grass programs. This file is
*   	    	usually included in every Grass program.
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
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

static const char *GRASS_copyright __attribute__((unused))
	= "GRASS GNU GPL licensed Software" ;

/* Define TRUE and FALSE for boolean comparisons */
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MAXEDLINES  50
#define RECORD_LEN  80
#define NEWLINE     '\n'
#define RECLASS_TABLE 1
#define RECLASS_RULES 2
#define RECLASS_SCALE 3

#define METERS    1
#define FEET      2
#define DEGREES   3

#define CELL_TYPE 0
#define FCELL_TYPE 1
#define DCELL_TYPE 2

#define PROJECTION_XY  0
#define PROJECTION_UTM 1
#define PROJECTION_SP  2
#define PROJECTION_LL  3
#define PROJECTION_OTHER  99

#define PROJECTION_FILE "PROJ_INFO"
#define UNIT_FILE "PROJ_UNITS"

/* define PI and friends */
#undef M_PI
#define M_PI    3.14159265358979323846 /* pi */

#undef M_PI_2
#define M_PI_2  1.57079632679489661923 /* pi/2 */

#undef M_PI_4
#define M_PI_4  0.78539816339744830962 /* pi/4 */

/* epsilon (IEEE: 2.220446e-16) */
#define GRASS_EPSILON 1.0e-15

/* Location of envariment variables */
#define G_VAR_GISRC    0
#define G_VAR_MAPSET   1

/* Where to find/store variables */
#define G_GISRC_MODE_FILE     0      /* files */
#define G_GISRC_MODE_MEMORY   1      /* memory only */

/* for G_parser() */
#define TYPE_INTEGER  1
#define TYPE_DOUBLE   2
#define TYPE_STRING   3
#define YES           1
#define NO            0
#define GISPROMPT_COLOR "color,grass,color"

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
#  define G_DEV_NULL 'NUL:'
#else
#  define HOST_DIRSEP '/'
#  define G_DEV_NULL '/dev/null'
#endif

/**/
typedef enum {
    G_OPT_WHERE,    /* SQL where conditions */
    G_OPT_TABLE,    /* table name */
    G_OPT_DRIVER,   /* driver name */
    G_OPT_DATABASE, /* database name */
    G_OPT_COLUMN,   /* one attr column */
    G_OPT_COLUMNS,  /* one or more attr columns */

    G_OPT_I_GROUP,  /* old input imagery group */
    G_OPT_R_INPUT,  /* old input raster map */
    G_OPT_R_INPUTS, /* old input raster maps */
    G_OPT_R_OUTPUT, /* new output raster map */
    G_OPT_R_MAP,    /* old input raster map */
    G_OPT_R_MAPS,   /* old input rasters map */
    G_OPT_R_BASE,   /* old input base raster map */
    G_OPT_R_COVER,  /* old input cover raster map */
    G_OPT_R_ELEV,   /* old input elevation raster map */
    G_OPT_R_ELEVS,  /* old input elevation raster maps */

    G_OPT_R3_INPUT,  /* old input raster3d map */
    G_OPT_R3_INPUTS, /* old input raster3d maps */
    G_OPT_R3_OUTPUT, /* new output raster3d map */
    G_OPT_R3_MAP,    /* old input raster3d map */
    G_OPT_R3_MAPS,   /* old input raster3d maps */

    G_OPT_V_INPUT,  /* old input vector map */
    G_OPT_V_INPUTS, /* old input vector maps */
    G_OPT_V_OUTPUT, /* new output vector map */
    G_OPT_V_MAP,    /* old input vector map */
    G_OPT_V_MAPS,    /* old input vector maps */
    G_OPT_V_TYPE,   /* primitive type */
    G_OPT_V_FIELD,  /* layer number (layers used to be called fields) */
    G_OPT_V_CAT,    /* one category */
    G_OPT_V_CATS,   /* more categories */

    G_OPT_F_INPUT,  /* old input file */
    G_OPT_F_OUTPUT, /* new output file */
    G_OPT_F_SEP,    /* data field separator */

    G_OPT_C_FG,     /* foreground color */
    G_OPT_C_BG      /* background color */
} STD_OPT;

/* Message format */
#define G_INFO_FORMAT_STANDARD 0  /* GRASS_MESSAGE_FORMAT=standard or not defined */
#define G_INFO_FORMAT_GUI      1  /* GRASS_MESSAGE_FORMAT=gui */
#define G_INFO_FORMAT_SILENT   2  /* GRASS_MESSAGE_FORMAT=silent */

/* Icon types */
#define G_ICON_CROSS  0
#define G_ICON_BOX    1
#define G_ICON_ARROW  2

/* default colors */
#define DEFAULT_FG_COLOR "black"
#define DEFAULT_BG_COLOR "white"

/* for G_get_raster_sample(), INTERP_TYPE */
#define UNKNOWN	  0
#define NEAREST   1  /* nearest neighbor interpolation	*/
#define BILINEAR  2  /* bilinear interpolation 		*/
#define CUBIC     3  /* cubic interpolation 		*/

/* for raster maps */
#define GR_FATAL_EXIT    0
#define GR_FATAL_PRINT   1
#define GR_FATAL_RETURN  2

/* Element types */
enum {            /* Dir */
    G_ELEMENT_RASTER      = 1,     /* cell */
    G_ELEMENT_RASTER3D    = 2,     /* 3dcell */
    G_ELEMENT_VECTOR      = 3,     /* vector */ 
    G_ELEMENT_OLDVECTOR   = 4,     /* GRASS < 5.7 vector */
    G_ELEMENT_ASCIIVECTOR = 5,     /* ASCII vector */
    G_ELEMENT_ICON        = 6,     /* icon */
    G_ELEMENT_LABEL       = 7,     /* labels */
    G_ELEMENT_SITE        = 8,     /* sites */
    G_ELEMENT_REGION      = 9,     /* region */
    G_ELEMENT_REGION3D    = 10,    /* 3dregion */
    G_ELEMENT_GROUP       = 11,    /* group */
    G_ELEMENT_3DVIEW      = 12     /* 3dview */
};

/*=========================== Typedefs/Structures ==========================*/

typedef int CELL;
typedef double DCELL;
typedef float FCELL;
extern CELL CELL_NODATA;

typedef int RASTER_MAP_TYPE;

/* for G_get_raster_sample() */
typedef int INTERP_TYPE;

struct Cell_head
{
    int format;     	/* max numer of bytes per cell minus 1          */
    int compressed; 	/* 0 = uncompressed, 1 = compressed, -1 pre 3.0 */
    int rows;	    	/* number of rows in the data 2D                */
    int rows3;	    	/* number of rows in the data 3D                */
    int cols;	    	/* number of columns in the data 2D             */
    int cols3;	    	/* number of columns in the data 3D             */
    int depths;         /* number of depths in data                     */
    int proj;	    	/* Projection (see #defines above)              */
    int zone;	    	/* Projection zone                              */
    double ew_res;  	/* East to West cell size 2D                    */
    double ew_res3;  	/* East to West cell size 3D                    */
    double ns_res;  	/* North to South cell size 2D                  */
    double ns_res3;  	/* North to South cell size 3D                  */
    double tb_res;  	/* Top to Bottom cell size                      */
    double north;   	/* coordinates of layer                         */
    double south;
    double east;
    double west;
    double top;
    double bottom;
};

struct _Color_Rule_
{
    struct
    {
    	DCELL value;
	unsigned char red;
	unsigned char grn;
	unsigned char blu;
    } low, high;

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
    int version;	/* set by read_colors: -1=old,1=new */
    DCELL shift;
    int invert;
    int is_float;   	    /* defined on floating point raster data? */
    int null_set;   	    /* the colors for null are set? */
    unsigned char null_red;
    unsigned char null_grn;
    unsigned char null_blu;
    int undef_set;  	    /* the colors for cells not in range are set? */
    unsigned char undef_red;
    unsigned char undef_grn;
    unsigned char undef_blu;
    struct _Color_Info_ fixed;
    struct _Color_Info_ modular;
    DCELL cmin;
    DCELL cmax;
};


typedef struct
{
    unsigned char r, g, b, a;  /* red, green, blue, and alpha */
} RGBA_Color ;

typedef RGBA_Color RGB_Color;

/* RGBA_Color alpha presets */
#define RGBA_COLOR_OPAQUE     255
#define RGBA_COLOR_TRANSPARENT  0
#define RGBA_COLOR_NONE         0


struct Reclass
{
    char *name;  	    /* name of raster map being reclassed    */
    char *mapset; 	    /* mapset in which "name" is found      */
    int type;	    	    /* type of reclass                      */
    int num;	    	    /* size of reclass table                */
    CELL min;	    	    /* table min    	    	    	    */
    CELL max;	    	    /* table max    	    	    	    */
    CELL *table;    	    /* reclass table                        */
} ;

struct FPReclass_table
{
    DCELL dLow;     /* domain low */
    DCELL dHigh;    /* domain high */
    DCELL rLow;     /* range low */
    DCELL rHigh;    /* range high */
};

/* reclass structure from double to double used by r.recode to reclass */
/* between types: int to double, float to int,... */
struct FPReclass
{
    int defaultDRuleSet;    /* 1 if default domain rule set */
    int defaultRRuleSet;    /* 1 if default range rule set */
    int infiniteLeftSet;    /* 1 if negative infinite interval rule exists */
    int infiniteRightSet;   /* 1 if positive infinite interval rule exists */
    int rRangeSet;  	    /* 1 if range range (i.e. interval) is set */
    int maxNofRules; 
    int nofRules;
    DCELL defaultDMin;      /* default domain minimum value */
    DCELL defaultDMax;      /* default domain maximum value */
    DCELL defaultRMin;      /* default range minimum value */
    DCELL defaultRMax;      /* default range maximum value */
    DCELL infiniteDLeft;    /* neg infinite rule */
    DCELL infiniteDRight;   /* neg infinite rule */
    DCELL infiniteRLeft;    /* pos infinite rule */
    DCELL infiniteRRight;   /* pos infinite rule */
    DCELL dMin;     	    /* minimum domain values in rules */
    DCELL dMax;     	    /* maximum domain values in rules */
    DCELL rMin;     	    /* minimum range values in rules */
    DCELL rMax;     	    /* maximum range values in rules */
    struct FPReclass_table *table;
};

struct Quant_table
{
    DCELL dLow;
    DCELL dHigh;
    CELL cLow;
    CELL cHigh;
};

struct Quant
{
    int truncate_only;
    int round_only;
    int defaultDRuleSet;
    int defaultCRuleSet;
    int infiniteLeftSet;
    int infiniteRightSet;
    int cRangeSet;
    int maxNofRules;
    int nofRules;
    DCELL defaultDMin;
    DCELL defaultDMax;
    CELL defaultCMin;
    CELL defaultCMax;
    DCELL infiniteDLeft;
    DCELL infiniteDRight;
    CELL infiniteCLeft;
    CELL infiniteCRight;
    DCELL dMin;
    DCELL dMax;
    CELL cMin;
    CELL cMax;
    struct Quant_table *table;

    struct
    {
	DCELL *vals;

	/* pointers to quant rules corresponding to the intervals btwn vals */
	struct Quant_table **rules;
	int nalloc;
	int active;
	DCELL inf_dmin; 
	DCELL inf_dmax; 
	CELL inf_min; 
	CELL inf_max; 
	/* all values smaller than inf_dmin become inf_min */
	/* all values larger than inf_dmax become inf_max */
	/* inf_min and/or inf_max can be NULL if there are no inf rules */
    } fp_lookup;
};

struct Categories
{
    CELL ncats;     	    /* total number of categories              */
    CELL num;	    	    /* the highest cell values. Only exists    
				 for backwards compatibility = (CELL)
				 max_fp_values in quant rules          */
    char *title;    	    /* name of data layer                      */ 
    char *fmt;	    	    /* printf-like format to generate labels   */
    float m1;	    	    /* Multiplication coefficient 1            */
    float a1;	    	    /* Addition coefficient 1                  */
    float m2;	    	    /* Multiplication coefficient 2            */
    float a2;	    	    /* Addition coefficient 2                  */
    struct Quant q; 	    /* rules mapping cell values to index in
				 list of labels                        */
    char **labels;  	    /* array of labels of size num             */	
    int *marks;    	    /* was the value with this label was used? */
    int nalloc;
    int last_marked_rule;
    /* NOTE: to get a rule corresponfing to cats.labels[i], use */
    /* G_get_ith_c/f/d_raster_cat (pcats, i, val1, val2) */
    /* it calls */
    /* G_quant_get_ith_rule(&cats->q, i, val1, val2, &index, &index); */
    /* and idex ==i, because rule is added at the same time as a */
    /* label, and quant rules are never reordered. Olga apr,95 */
};

struct History
{
    char    mapid[RECORD_LEN];
    char    title[RECORD_LEN];
    char    mapset[RECORD_LEN];
    char    creator[RECORD_LEN];
    char    maptype[RECORD_LEN];
    char    datsrc_1[RECORD_LEN];
    char    datsrc_2[RECORD_LEN];
    char    keywrd[RECORD_LEN];
    int     edlinecnt;
    char    edhist[MAXEDLINES][RECORD_LEN];
};

struct Cell_stats
{
    struct Cell_stats_node
    {
	int idx;
	long *count;
	int left;
	int right;
    } *node ;     /* tree of values */
 
    int tlen ;    /* allocated tree size */
    int N;        /* number of actual nodes in tree */
    int curp;
    long null_data_count;   
    int curoffset;
};

struct Histogram
{
    int num;
    
    struct Histogram_list
    {
	CELL cat;
	long count;
    } *list;
};

struct Range
{
    CELL min;
    CELL max;
    int first_time;     /* whether or not range was updated */
};

struct FPRange
{
    DCELL min;
    DCELL max;
    int first_time;     /* whether or not range was updated */
};

/*
** Structure for I/O of 3dview files  (view.c)
*/
struct G_3dview
{
    char pgm_id[40];        /* user-provided identifier */
    float from_to[2][3];    /* eye position & lookat position */
    float fov;              /* field of view */
    float twist;            /* right_hand rotation about from_to */
    float exag;             /* terrain elevation exageration */
    int mesh_freq;  	    /* cells per grid line */
    int poly_freq;   	    /* cells per polygon */
    int display_type;       /* 1 for mesh, 2 for poly, 3 for both */
    int lightson;   	    /* boolean */
    int dozero;   	    /* boolean */
    int colorgrid;   	    /* boolean */
    int shading;   	    /* boolean */
    int fringe;   	    /* boolean */
    int surfonly;   	    /* boolean */
    int doavg;   	    /* boolean */
    char grid_col[40];	    /* colors */
    char bg_col[40];	    /* colors */
    char other_col[40];     /* colors */
    float lightpos[4];	    /* east, north, height, 1.0 for local 0.0 infin */
    float lightcol[3];       /* values between 0.0 to 1.0 for red, grn, blu */
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

struct Option                	    /* Structure that stores option info */
{
    char *key;                      /* Key word used on command line    */
    int type;                       /* Option type                      */
    int required;                   /* REQUIRED or OPTIONAL             */
    int multiple;                   /* Multiple entries OK              */
    char *options;                  /* Approved values or range or NULL */
    char **opts;                    /* NULL or NULL terminated array of parsed options */
    char *key_desc;                 /* one word describing the key      */
    char *label;                    /* Optional short label, used in GUI as item label */
    char *description;              /* String describing option         */
    char *descriptions;             /* ';' separated pairs of option and option descriptions */
				    /* For example: (with ->options = "break,rmdupl")
				     * "break;break lines on intersections;"
				     * "rmdupl;remove duplicates"
				     */
    char **descs;                   /* parsed descriptions, array of either NULL or string */
                                    /* in the same order as options */
    char *answer;                   /* Option answer                    */
    char *def;                      /* Where original answer gets saved */
    char **answers;                 /* Option answers (for multiple=YES)*/
    struct Option *next_opt;        /* Pointer to next option struct    */
    char *gisprompt;                /* Interactive prompt guidance      */
    char *guisection;               /* GUI Layout guidance: ';' delimited heirarchical tree position */
    int (*checker)();               /* Routine to check answer or NULL  */
    int count;
};

struct Flag                 	    /* Structure that stores flag info  */
{
    char key;                       /* Key char used on command line    */
    char answer;                    /* Stores flag state: 0/1           */
    char *label;                    /* Optional short label, used in GUI as item label */
    char *description;              /* String describing flag meaning   */
    char *guisection;               /* GUI Layout guidance: ';' delimited heirarchical tree position */
    struct Flag *next_flag;         /* Pointer to next flag struct      */
};

struct GModule                      /* Structure that stores module info  */
{
    char *label;                    /* Optional short description for GUI */
    char *description;              /* String describing module */
    char *keywords;                 /* Keywords describing module */
	/* further items are possible: author(s), version */
    int overwrite;                  /* overwrite old files */
    int verbose;                    /* print all informations about progress and so on */
};

struct TimeStamp
{
    DateTime dt[2];   /* two datetimes */
    int count;
};

/*============================== Prototypes ================================*/

/* Since there are so many prototypes for the gis library they are stored */
/* in the file gisdefs.h */
#include <grass/gisdefs.h>

#endif /* GRASS_GIS_H */
