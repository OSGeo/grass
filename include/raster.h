#ifndef GRASS_RASTER_H
#define GRASS_RASTER_H

#include <grass/gis.h>

/*** defines ***/
#define RECLASS_TABLE 1
#define RECLASS_RULES 2
#define RECLASS_SCALE 3

#define CELL_TYPE 0
#define FCELL_TYPE 1
#define DCELL_TYPE 2

/*! \brief Interpolation methods

  For G_get_raster_sample(), INTERP_TYPE

  \todo Rename to use prefix INTERP_
*/
#define UNKNOWN	  0
#define NEAREST   1		/* nearest neighbor interpolation  */
#define BILINEAR  2		/* bilinear interpolation          */
#define CUBIC     3		/* cubic interpolation             */

/*** typedefs ***/
typedef int RASTER_MAP_TYPE;

/* for G_get_raster_sample() */
typedef int INTERP_TYPE;

/*** structures ***/
struct Reclass
{
    char *name;			/* name of raster map being reclassed    */
    char *mapset;		/* mapset in which "name" is found      */
    int type;			/* type of reclass                      */
    int num;			/* size of reclass table                */
    CELL min;			/* table min                            */
    CELL max;			/* table max                            */
    CELL *table;		/* reclass table                        */
};

struct FPReclass_table
{
    DCELL dLow;			/* domain low */
    DCELL dHigh;		/* domain high */
    DCELL rLow;			/* range low */
    DCELL rHigh;		/* range high */
};

/* reclass structure from double to double used by r.recode to reclass */
/* between types: int to double, float to int,... */
struct FPReclass
{
    int defaultDRuleSet;	/* 1 if default domain rule set */
    int defaultRRuleSet;	/* 1 if default range rule set */
    int infiniteLeftSet;	/* 1 if negative infinite interval rule exists */
    int infiniteRightSet;	/* 1 if positive infinite interval rule exists */
    int rRangeSet;		/* 1 if range range (i.e. interval) is set */
    int maxNofRules;
    int nofRules;
    DCELL defaultDMin;		/* default domain minimum value */
    DCELL defaultDMax;		/* default domain maximum value */
    DCELL defaultRMin;		/* default range minimum value */
    DCELL defaultRMax;		/* default range maximum value */
    DCELL infiniteDLeft;	/* neg infinite rule */
    DCELL infiniteDRight;	/* neg infinite rule */
    DCELL infiniteRLeft;	/* pos infinite rule */
    DCELL infiniteRRight;	/* pos infinite rule */
    DCELL dMin;			/* minimum domain values in rules */
    DCELL dMax;			/* maximum domain values in rules */
    DCELL rMin;			/* minimum range values in rules */
    DCELL rMax;			/* maximum range values in rules */
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
    CELL ncats;			/* total number of categories              */
    CELL num;			/* the highest cell values. Only exists    
				   for backwards compatibility = (CELL)
				   max_fp_values in quant rules          */
    char *title;		/* name of data layer                      */
    char *fmt;			/* printf-like format to generate labels   */
    float m1;			/* Multiplication coefficient 1            */
    float a1;			/* Addition coefficient 1                  */
    float m2;			/* Multiplication coefficient 2            */
    float a2;			/* Addition coefficient 2                  */
    struct Quant q;		/* rules mapping cell values to index in
				   list of labels                        */
    char **labels;		/* array of labels of size num             */
    int *marks;			/* was the value with this label was used? */
    int nalloc;
    int last_marked_rule;
    /* NOTE: to get a rule corresponfing to cats.labels[i], use */
    /* G_get_ith_c/f/d_raster_cat (pcats, i, val1, val2) */
    /* it calls */
    /* G_quant_get_ith_rule(&cats->q, i, val1, val2, &index, &index); */
    /* and idex ==i, because rule is added at the same time as a */
    /* label, and quant rules are never reordered. Olga apr,95 */
};

/*! \brief Raster history info (metadata)

  See History structure for implementation issues.
*/
enum History_field
{
    /*! \brief Raster name */
    HIST_MAPID,
    /*! \brief Raster title */
    HIST_TITLE,
    /*! \brief Raster mapset */
    HIST_MAPSET,
    /*! \brief User who creater raster map */
    HIST_CREATOR,
    /*! \brief Map type (always "raster") */
    HIST_MAPTYPE,
    /*! \brief Description of original data source (two lines) */
    HIST_DATSRC_1,
    HIST_DATSRC_2,
    /*! \brief One-line data description */
    HIST_KEYWRD,

    /*! \brief Number of fields to be defined in History structure */
    HIST_NUM_FIELDS,
};

/*! \brief Raster history info (metadata) */
struct History
{
    /*! \brief Array of fields (see \ref History_field for details) */
    char *fields[HIST_NUM_FIELDS];
    /*! \brief Number of lines in lines array */
    int nlines;
    /*! \brief Lines array */
    char **lines;
};

struct Cell_stats
{
    struct Cell_stats_node
    {
	int idx;
	long *count;
	int left;
	int right;
    } *node;			/* tree of values */

    int tlen;			/* allocated tree size */
    int N;			/* number of actual nodes in tree */
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
    int first_time;		/* whether or not range was updated */
};

struct FPRange
{
    DCELL min;
    DCELL max;
    int first_time;		/* whether or not range was updated */
};

struct FP_stats {
    int geometric;
    int geom_abs;
    int flip;
    int count;
    DCELL min, max;
    unsigned long *stats;
    unsigned long total;
};

struct GDAL_link;

typedef struct
{
    unsigned char r, g, b, a;	/* red, green, blue, and alpha */
} RGBA_Color;

typedef RGBA_Color RGB_Color;

/* RGBA_Color alpha presets */
#define RGBA_COLOR_OPAQUE     255
#define RGBA_COLOR_TRANSPARENT  0
#define RGBA_COLOR_NONE         0

/*** prototypes ***/
#include <grass/defs/raster.h>

#endif /* GRASS_RASTER_H */
