/*
   **  Written by Dave Gerdes  5/1988
   **  US Army Construction Engineering Research Lab
 */
#include <grass/config.h>

#ifndef  DIG___STRUCTS___
#define DIG___STRUCTS___

/*  this file depends on  <stdio.h> */
#ifndef _STDIO_H
#include <stdio.h>
#endif

#include <grass/dgl.h>
#include <grass/shapefil.h>
#include <grass/btree.h>
#include <grass/rtree.h>


#ifdef HAVE_OGR
#include "ogr_api.h"
#endif

#define HEADSTR	50

/*
   **  NOTE: 3.10 changes plus_t to  ints.
   **    This assumes that any reasonable machine will use 4 bytes to
   **    store an int.  The mapdev code is not guaranteed to work if
   **    plus_t is changed to a type that is larger than an int.
 */
/*
   typedef short plus_t;
 */
typedef int plus_t;

#define BOUND_BOX struct bound_box

#define P_NODE struct P_node
#define P_AREA struct P_area
#define P_LINE struct P_line
#define P_ISLE struct P_isle

/* Used by sites lib */
typedef struct {
    int cat;    /* category */
    double *dbl;   /* double attributes */
    char **str;   /* string attributes */
} SITE_ATT;

struct bound_box        /* Bounding Box */
  {
    double N;	/* north */			
    double S;   /* south */
    double E;   /* east */
    double W;   /* west */
    double T;   /* top */
    double B;   /* bottom */
  };

typedef struct {
    FILE *file;
    char *start;   /* pointer to beginnig of the file in the memory */
    char *current; /* current position set by dig_seek */
    char *end;     /* end of file in the memory (pointer to first byte after) */
    long size;     /* size of the file loaded to memory */
    long alloc;    /* allocated space */
    int  loaded;   /* 0 - not loaded, 1 - loaded */
} GVFILE;

/* category field information */
struct field_info
  {
    int  number;   /* field number */
    char *name;    /* field name */
    char *driver;
    char *database;
    char *table;
    char *key;
  };

struct dblinks 
  {
    struct field_info *field;
    int alloc_fields, n_fields;
  };

/* Portability info */
struct Port_info
  {	  
    /* portability stuff, set in V1_open_new/old() */
    /* file byte order */
    int byte_order; 
      
    /* conversion matrices between file and native byte order */
    unsigned char dbl_cnvrt[PORT_DOUBLE];
    unsigned char flt_cnvrt[PORT_FLOAT];
    unsigned char lng_cnvrt[PORT_LONG];
    unsigned char int_cnvrt[PORT_INT];
    unsigned char shrt_cnvrt[PORT_SHORT];
    
    /* *_quick specify if native byte order of that type 
     * is the same as byte order of vector file (TRUE) 
     * or not (FALSE);*/
    int dbl_quick;
    int flt_quick;
    int lng_quick;
    int int_quick;
    int shrt_quick;
  };

/* List of dead lines in the file, the space can be reused, not yet used */
struct recycle
  {
     /* TODO */
     char dummy;
  };

struct dig_head
  {	  
    /*** HEAD_ELEMENT ***/
    char *organization;
    char *date;
    char *your_name;
    char *map_name;
    char *source_date;
    long orig_scale;
    char *line_3;
    int plani_zone;
    /* double W, E, S, N; */
    double digit_thresh;
    /* double map_thresh; */ /* not used in g57 */

    /* Programmers should NOT touch any thing below here */
    /* Library takes care of everything for you          */
    /*** COOR_ELEMENT ***/
    int Version_Major;
    int Version_Minor;
    int Back_Major;
    int Back_Minor;
    int with_z;
    
    long size;                  /* coor file size */
    long head_size;             /* coor header size */

    struct Port_info port;      /* Portability information */
   
    long last_offset;           /* offset of last read line */

    struct recycle *recycle;
    
    struct Map_info *Map;	/* X-ref to Map_info struct ?? */
  };

/* Coor info */
struct Coor_info
  {	  
    long size;     /* total size, in bytes */ 
    long mtime;    /* time of last modification */
  };

/* Non-native format inforamtion */
/* TODO: structure size should not change depending on compilation I think, do it better */
/* OGR */
struct Format_info_ogr {
    char           *dsn;
    char           *layer_name;
#ifdef HAVE_OGR
    OGRDataSourceH ds;
    OGRLayerH      layer;
#else
    void *ds;
    void *layer;
#endif
    
    /* Level 1 (used by V*_read_next_line_ogr) */
    struct line_pnts **lines;  /* points cache */
    int *lines_types; 
    int lines_alloc;
    int lines_num;             /* number of lines in cache */
    int lines_next; /* next line to be read from cache */

    /* Level 2 */
#ifdef HAVE_OGR
    OGRFeatureH    feature_cache; /* cache to avoid repeated reading,  NULL if no feature is in cache */
#else
    void *feature_cache;
#endif
    int            feature_cache_id; /* id of feature read in feature_cache */

    /* Array where OGR feature/part info is stored for each line in GRASS.
     * This is not used for GV_CENTROID.
     * Because one feature may contain more elements (geometry collection also recursively),
     * offset for one line may be stored in more records. 
     * First record is FID, next records are part indexes if necessary.
     * Example:
     * 5. ring in 3. polygon in 7. feature (multipolygon) of geometry collection which has FID = 123
     * 123 (feature 123: geometry colletion)
     *   6 (7. feature in geometry collection: multiPolygon)
     *   2 (3. polygon)
     *   4 (5. ring in the polygon)
     */  
    int *offset; 
    int offset_num; /* number of items in offset */
    int offset_alloc; /* space allocated for offset */

    int next_line; /* used by V2_read_next_line_ogr */
} ;

struct Format_info {
    int i;
    struct Format_info_ogr ogr;
} ;

/* Category index */
struct Cat_index {
    int field; /* field number */
    int n_cats;    /* number of items in cat array */ 
    int a_cats;    /* allocated space in cat array */
    int (*cat)[3]; /* array of cats (cat,type, lines/area) */ 
    int n_ucats;  /* number of unique cats (not updated) */
    int n_types; /* number of types in type */
    int type[7][2]; /* number of elements for each type (point, line, boundary, centroid, area, face, kernel) */
    long offset; /* offset of the beginning of this index in cidx file */
};

struct Plus_head
  {
    int Version_Major;		/* version codes */
    int Version_Minor;
    int Back_Major;		/* earliest version that can use this data format */
    int Back_Minor;
    
    int spidx_Version_Major;		/* version codes for spatial index */
    int spidx_Version_Minor;
    int spidx_Back_Major;		/* earliest version that can use this data format */
    int spidx_Back_Minor;
    
    int cidx_Version_Major;	/* version codes for category index */
    int cidx_Version_Minor;
    int cidx_Back_Major;	/* earliest version that can use this data format */
    int cidx_Back_Minor;
    
    int with_z;
    int spidx_with_z;

    long head_size;             /* topo header size */
    long spidx_head_size;       /* spatial index header size */
    long cidx_head_size;        /* category index header size */
    
    int  release_support;      /* release memory occupied by support (topo, spatial, category) */

    struct Port_info port;        /* Portability information */
    struct Port_info spidx_port;  /* Portability information for spatial index */
    struct Port_info cidx_port;   /* Portability information for category index */
    int mode;			/* Read, Write, RW */

    int built;                 /* the highest level of topology currently available (GV_BUILD_*) */

    struct bound_box box;      /* box */
    
    P_NODE **Node;	/* P_NODE array of pointers *//* 1st item is 1 for  */
    P_LINE **Line;	/* P_LINE array of pointers *//* all these (not 0) */
    P_AREA **Area;		
    P_ISLE **Isle;
   
    plus_t n_nodes;		/* Current Number of nodes */
    plus_t n_edges;		/* Current Number of edges */
    plus_t n_lines;		/* Current Number of lines */
    plus_t n_areas;		/* Current Number of areas */
    plus_t n_isles;
    plus_t n_volumes;		/* Current Number of volumes */
    plus_t n_holes;		/* Current Number of holes */

    plus_t n_plines;		/* Current Number of point    lines */
    plus_t n_llines;		/* Current Number of line     lines */
    plus_t n_blines;		/* Current Number of boundary lines */
    plus_t n_clines;		/* Current Number of centroid lines */
    plus_t n_flines;		/* Current Number of face lines */
    plus_t n_klines;		/* Current Number of kernel lines */

    plus_t alloc_nodes;		/* # of nodes we have alloc'ed space for 
				     i.e. array size - 1 */
    plus_t alloc_edges;
    plus_t alloc_lines;		/* # of lines we have alloc'ed space for */
    plus_t alloc_areas;		/* # of areas we have alloc'ed space for */
    plus_t alloc_isles;		/* # of isles we have alloc'ed space for */
    plus_t alloc_volumes;
    plus_t alloc_holes;

    long Node_offset;           /* offset of array of nodes in topo file */
    long Edge_offset;
    long Line_offset;
    long Area_offset;
    long Isle_offset;
    long Volume_offset;
    long Hole_offset;

    /* Spatial index */
    /* Spatial index is never saved, it is built automaticaly for new and updated vectors.
     * It is not built for old vectors until it is needed, i.e. until Vect_select is called. 
     * or until Vect_build is called */
    
    int Spidx_built;   /* set to 1 if spatial index is available and to 0 if it is not */

    long Node_spidx_offset;     /* offset of spindex */
    long Edge_spidx_offset;
    long Line_spidx_offset;
    long Area_spidx_offset;
    long Isle_spidx_offset;
    long Volume_spidx_offset;
    long Hole_spidx_offset;
    
    struct Node *Node_spidx;
    struct Node *Line_spidx;
    struct Node *Area_spidx;
    struct Node *Isle_spidx;

    /* Category index */
    /* By default, category index is not updated */
    int update_cidx; /* update category index if vector is modified */
    
    int    n_cidx; /* number of cat indexes (one for each field) */
    int    a_cidx; /* allocated space for cat indexes */
    struct Cat_index *cidx; /* Array of category indexes */
    int    cidx_up_to_date; /* set to 1 when cidx is created and reset to 0 whenever any line is changed */

    long coor_size;		/* size of coor file */
    long coor_mtime;		/* time of last coor modification */

    /* Level2 update: list of lines and nodes updated (topo info for the line was changed) 
    *                 by last write/rewrite/delete operation.
    *                 Lines/nodes in the list may be deleted (e.g. delete boundary: first added for
    *                 delete area and then delete */
    int do_uplist;     /* used internaly in diglib to know if list is maintained */
    
    int *uplines;      /* array of updated lines */
    int alloc_uplines; /* allocated array */
    int n_uplines;     /* number of updated lines */
    int *upnodes;      /* array of updated nodes */
    int alloc_upnodes; /* allocated array */
    int n_upnodes;     /* number of updated nodes */
  };

struct Map_info
  {
    /* Common info for all formats */  
    int format;                 /* format */
    int temporary;              /* temporary file flag, not yet used */

    struct dblinks *dblnk;      /* info about tables */ 
    
    struct Plus_head plus;      /* topo file *head; */

    int graph_line_type;        /* line type used to build the graph */
    dglGraph_s   graph;       	/* graph structure */
    dglSPCache_s spCache;       /* Shortest path cache */ 
    double *edge_fcosts;        /* costs used for graph, (dglGetEdge() is not supported for _DGL_V1) */
    double *edge_bcosts;        
    double *node_costs;         /* node costs */
    int cost_multip;            /* edge and node costs multiplicator */

    /*  All of these apply only to runtime, and none get written out
    **  to the dig_plus file 
    */
    int open;			/* should be 0x5522AA22 if opened correctly */
                                /* or        0x22AA2255 if closed           */
                                /* anything else implies that structure has */
                                /* never been initialized                   */
    int mode;			/*  Read, Write, RW                         */
    int level;			/*  1, 2, (3)                               */
    int head_only;              /* Only header is opened */ 
    int support_updated;        /* Support files were updated */
    plus_t next_line;		/* for Level II sequential reads */

    char *name;			/* for 4.0  just name, and mapset */
    char *mapset;
    /* location and gisdbase is usefull if changed (v.proj or external apps) */
    char *location;    /* location name */
    char *gisdbase;    /* gisdbase path */

    /* Constraints for reading in lines  (not polys yet) */
    int    Constraint_region_flag;
    int    Constraint_type_flag;
    double Constraint_N;
    double Constraint_S;
    double Constraint_E;
    double Constraint_W;
    double Constraint_T;
    double Constraint_B;
    int    Constraint_type;
    int    proj;

    /* format specific */
    /* native */
    GVFILE dig_fp;		/* Dig file pointer */
    struct dig_head head;	/* coor file head */
    
    /* non native */
    struct Format_info fInfo;  /* format information */

    /* history file */
    FILE *hist_fp;

    /* Temporary solution for sites */
    SITE_ATT *site_att;  /* array of attributes loaded from db */
    int n_site_att;      /* number of attributes in site_att array */
    int n_site_dbl;      /* number of double attributes for one site */
    int n_site_str;      /* number of string attributes for one site */
  };

struct P_node
  {
    double x;			/* X coordinate */
    double y;			/* Y coordinate */
    double z;			/* Z coordinate */
    plus_t alloc_lines;  
    plus_t n_lines;	/* Number of attached lines (size of lines, angle) */
    /*  If 0, then is degenerate node, for snappingi ??? */
    plus_t *lines;		/* Connected lines */
    float  *angles;		/* Respected angles. Angles for lines/boundaries are in radians between
				 * -PI and PI. Value for points or lines with identical points (degenerated)
				 * is set to -9. */  
  };

struct P_line
  {
    plus_t N1;		/* start node */
    plus_t N2;		/* end node   */
    plus_t left;	/* area/isle number to left, negative for isle  */
                        /* !!! area number for centroid, negative for   */
			/* duplicate centroid                           */ 
    plus_t right;	/* area/isle number to right, negative for isle */

    double N;		/* Bounding Box */
    double S;
    double E;
    double W;
    double T;           /* top */
    double B;           /* bottom */ 

    long offset;	/* offset in coor file for line */
    int  type;	
  };

struct P_area
  {
    double N;		/* Bounding Box */
    double S;
    double E;
    double W;
    double T;           /* top */
    double B;           /* bottom */ 
    plus_t n_lines;	/* Number of boundary lines */
    plus_t alloc_lines;
    plus_t *lines;	/* Boundary Lines, negative means direction N2 to N1,
			   lines are in  clockwise order */
    
    /*********  Above this line is compatible with P_isle **********/
    
    plus_t centroid;		/* Number of first centroid within area */

    plus_t n_isles;		/* Number of islands inside */
    plus_t alloc_isles;
    plus_t *isles;		/* 1st generation interior islands */
  };

struct P_isle
  {
    double N;		/* Bounding Box */
    double S;
    double E;
    double W;
    double T;           /* top */
    double B;           /* bottom */ 
    plus_t n_lines;	/* Number of boundary lines */
    plus_t alloc_lines;
    plus_t *lines;	/* Boundary Lines, negative means direction N2 to N1,
			   lines are in counter clockwise order */

    /*********  Above this line is compatible with P_area **********/

    plus_t area;	/* area it exists w/in, if any */
  };

struct line_pnts
  {
    double *x;
    double *y;
    double *z;
    int n_points;
    int alloc_points;
  };

struct line_cats
  {
    int *field;		/* pointer to array of fields */
    int *cat;		/* pointer to array of categories */
    int n_cats;		/* number of vector categories attached to element */
    int alloc_cats;	/* allocated space */
  };

struct cat_list
  {
    int field;        /* category field */	  
    int *min;         /* pointer to array of minimun values */
    int *max;         /* pointer to array of maximum values */
    int n_ranges;     /* number ranges */
    int alloc_ranges; /* allocated space */
  };

/* list of integers */
struct ilist
  {
    int    *value;       /* items */	  
    int    n_values;     /* number of values */
    int    alloc_values; /* allocated space */
  };

/* Vector array. Space allocated is size + 1. */
typedef struct {
    int size;    /* array size */
    int *c;      /* array where 'class' or new category or something like that is stored */
} VARRAY;

/* Spatial index for use in modules. */
typedef struct {
    struct Node *root;
} SPATIAL_INDEX;

typedef dglGraph_s GRAPH;     	/* graph structure */
    
#endif /* DIG___STRUCTS___ */

