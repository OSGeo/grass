/*!
  \file include/vect/dig_structs.h

  \brief Structures for diglib (vector library)

  \author Written by Dave Gerdes (CERL)  5/1988
  \author Updated to GRASS 5.7 by Radim Blazek (2001)
*/
#include <grass/config.h>

#ifndef  DIG___STRUCTS___
#define DIG___STRUCTS___

/*  this file depends on <stdio.h> */
#ifndef _STDIO_H
#include <stdio.h>
#endif

#include <sys/types.h>

#include <grass/dgl.h>
#include <grass/shapefil.h>
#include <grass/rbtree.h>
#include <grass/rtree.h>

#ifdef HAVE_OGR
#include "ogr_api.h"
#endif

/*! \brief plus_t size

  3.10 changes plus_t to  ints.
  This assumes that any reasonable machine will use 4 bytes to
  store an int. The diglib code is not guaranteed to work if
  plus_t is changed to a type that is larger than an int.
*/
typedef int plus_t;

/*! \brief Used by sites lib */
struct site_att
{
    int cat;			/* category */
    double *dbl;		/* double attributes */
    char **str;			/* string attributes */
};

/*! \brief Bounding box */
struct bound_box
{
    double N;			/* north */
    double S;			/* south */
    double E;			/* east */
    double W;			/* west */
    double T;			/* top */
    double B;			/* bottom */
};

/*! \brief File definition */
struct gvfile
{
    FILE *file;
    char *start;		/* pointer to beginnig of the file in the memory */
    char *current;		/* current position set by dig_seek() */
    char *end;			/* end of file in the memory (pointer to first byte after) */
    off_t size;			/* size of the file loaded to memory */
    off_t alloc;		/* allocated space */
    int loaded;			/* 0 - not loaded, 1 - loaded */
};

/*! \brief Field information */
struct field_info
{
    int number;			/* field number */
    char *name;			/* field name */
    char *driver;               /* db driver */
    char *database;             /* db database */
    char *table;                /* db table */
    char *key;                  /* table key */
};

/*! \brief Database links */
struct dblinks
{
    struct field_info *field;   /* pointer to the first field_info structure */
    int alloc_fields;           /* number of allocated slots */
    int n_fields;               /* number of available fields */
};

/*! \brief Portability info - set in V1_open_new/old() */
struct Port_info
{
    int byte_order;             /* file byte order */
    int off_t_size;             /* off_t size */

    /* conversion matrices between file and native byte order */
    unsigned char dbl_cnvrt[PORT_DOUBLE];
    unsigned char flt_cnvrt[PORT_FLOAT];
    unsigned char lng_cnvrt[PORT_LONG];
    unsigned char int_cnvrt[PORT_INT];
    unsigned char shrt_cnvrt[PORT_SHORT];
    unsigned char off_t_cnvrt[PORT_OFF_T];

    /* *_quick specify if native byte order of that type is the same
      as byte order of vector file (TRUE) or not (FALSE) */
    int dbl_quick;
    int flt_quick;
    int lng_quick;
    int int_quick;
    int shrt_quick;
    int off_t_quick;
};

/*! \brief List of dead lines in the file, the space can be reused,
  not yet used

  \todo Implement it
*/
struct recycle
{
    char dummy;
};

/*! \brief Vector map head */
struct dig_head
{
    /* elements */
    char *organization;         /* orgranization name */
    char *date;                 /* map date */
    char *your_name;            /* user name */
    char *map_name;             /* map name */
    char *source_date;          /* source date */
    long orig_scale;            /* original scale */
    char *line_3;               /* comments */
    int plani_zone;             /* zone */
    double digit_thresh;        /* threshold for digitization */

    /* Programmers should NOT touch any thing below here */
    /* Library takes care of everything for you          */

    /* coor elements */
    int Version_Major;          /* backward compatibility info */
    int Version_Minor;
    int Back_Major;
    int Back_Minor;
    int with_z;                 /* 2D/3D vector data */

    off_t size;			/* coor file size */
    long head_size;		/* coor header size */

    struct Port_info port;	/* portability information */

    off_t last_offset;		/* offset of last read line */

    struct recycle *recycle;    /* recycle dead line, not implemented yet */
};

/*! \brief Coor file info */
struct Coor_info
{
    off_t size;			/* total size, in bytes */
    long mtime;			/* time of last modification */
};

/*! \brief Non-native format info (OGR)

  \todo Structure size should not change depending on compilation I
think, do it better */
struct Format_info_ogr
{
    char *driver_name;          /* OGR driver name */
    char *dsn;                  /* OGR datasource name */
    char *layer_name;           /* OGR layer name */
#ifdef HAVE_OGR
    OGRSFDriverH driver;        /* pointer to OGRDriver */
    OGRDataSourceH ds;          /* pointer to OGRDataSourceH */
    OGRLayerH layer;            /* pointer to OGRLayerH */
#else
    void *driver;
    void *ds;
    void *layer;
#endif
    
    char **dsn_options;
    char **layer_options;
    
    /* Level 1 (used by V*_read_next_line_ogr) */
    struct line_pnts **lines;	/* points cache */
    int *lines_types;           /* list of line types */
    int lines_alloc;            /* number of allocated lines */
    int lines_num;		/* number of lines in cache */
    int lines_next;		/* next line to be read from cache */

    /* Level 2 */
#ifdef HAVE_OGR
    OGRFeatureH feature_cache;	/* cache to avoid repeated reading,  NULL if no feature is in cache */
#else
    void *feature_cache;
#endif
    int feature_cache_id;	/* id of feature read in feature_cache */

    /* Array where OGR feature/part info is stored for each line in
     * GRASS. This is not used for GV_CENTROID. Because one feature
     * may contain more elements (geometry collection also
     * recursively), offset for one line may be stored in more
     * records. First record is FID, next records are part indexes if
     * necessary. Example: 5. ring in 3. polygon in 7. feature
     * (multipolygon) of geometry collection which has FID = 123 123
     * (feature 123: geometry colletion) 6 (7. feature in geometry
     * collection: multiPolygon) 2 (3. polygon) 4 (5. ring in the
     * polygon)
     */
    int *offset;                /* offset list */
    int offset_num;		/* number of items in offset */
    int offset_alloc;		/* space allocated for offset */

    int next_line;		/* used by V2_read_next_line_ogr */
};

/*! \brief Non-native format info */
struct Format_info
{
    int i;                      /* id? */
    struct Format_info_ogr ogr; /* OGR info */
};

/*! \brief Category index */
struct Cat_index
{
    int field;			/* field number */
    int n_cats;			/* number of items in cat array */
    int a_cats;			/* allocated space in cat array */
    int (*cat)[3];		/* array of cats (cat, type, lines/area) */
    int n_ucats;		/* number of unique cats (not updated) */
    int n_types;		/* number of types in type */
    int type[7][2];		/* number of elements for each type
				 * (point, line, boundary, centroid,
				 * area, face, kernel) */
    off_t offset;		/* offset of the beginning of this
				 * index in cidx file */
};

/*! \brief Plus head info */
struct Plus_head
{
    /*** version info ***/
    int Version_Major;		 /* version codes */
    int Version_Minor;
    int Back_Major;		 /* earliest version that can use this data format */
    int Back_Minor;

    int spidx_Version_Major;	 /* version codes for spatial index */
    int spidx_Version_Minor;
    int spidx_Back_Major;	 /* earliest version that can use this data format */
    int spidx_Back_Minor;

    int cidx_Version_Major;	 /* version codes for category index */
    int cidx_Version_Minor;
    int cidx_Back_Major;	 /* earliest version that can use this data format */
    int cidx_Back_Minor;

    int with_z;                  /* 2D/3D vector data */
    int spidx_with_z;            /* 2D/3D spatial index */

    int off_t_size;              /* offset size here because Plus_head
				    is available to all releveant
				    functions */

    /*** file header size ***/
    long head_size;		 /* topo header size */
    long spidx_head_size;	 /* spatial index header size */
    long cidx_head_size;	 /* category index header size */

    int release_support;	 /* release memory occupied by support
				    (topo, spatial, category) */

    /*** portability info */
    struct Port_info port;	 /* portability information */
    struct Port_info spidx_port; /* portability information for
				    spatial index */
    struct Port_info cidx_port;	 /* portability information for
				    category index */
    int mode;			 /* read, write, RW */

    int built;			 /* the highest level of topology
				    currently available
				    (GV_BUILD_*) */

    struct bound_box box;	 /* vector map bounding box */

    /*** topology ***/
    struct P_node **Node;	 /* struct P_node array of pointers
				    1st item is 1 for  */
    struct P_line **Line;	 /* struct P_line array of pointers
				    all these (not 0) */
    struct P_area **Area;
    struct P_isle **Isle;
                                 /* add here P_FACE, P_VOLUME, P_HOLE */

    plus_t n_nodes;		 /* current number of nodes */
    plus_t n_edges;		 /* current number of edges */
    plus_t n_lines;	 	 /* current number of lines */
    plus_t n_areas;		 /* current number of areas */
    plus_t n_isles;		 /* current number of isles */
    plus_t n_faces;		 /* current number of faces */
    plus_t n_volumes;	         /* current number of volumes */
    plus_t n_holes;		 /* current number of holes */

    plus_t n_plines;		 /* current number of points */
    plus_t n_llines;		 /* current number of lines */
    plus_t n_blines;		 /* current number of boundaries */
    plus_t n_clines;		 /* current number of centroids */
    plus_t n_flines;		 /* current number of faces */
    plus_t n_klines;		 /* current number of kernels*/
    plus_t n_vfaces;		 /* current number of volume faces */
    plus_t n_hfaces;		 /* current number of hole faces */

    plus_t alloc_nodes;		 /* number of nodes we have alloc'ed
				    space for i.e. array size - 1 */
    plus_t alloc_edges;          /* number of edges we have alloc'ed space for */
    plus_t alloc_lines;		 /* number of lines we have alloc'ed space for */
    plus_t alloc_areas;		 /* number of areas we have alloc'ed space for */
    plus_t alloc_isles;		 /* number of isles we have alloc'ed space for */
    plus_t alloc_faces;		 /* number of faces we have alloc'ed space for */
    plus_t alloc_volumes;	 /* number of volumes we have alloc'ed space for */
    plus_t alloc_holes;		 /* number of holes we have alloc'ed space for */

    off_t Node_offset;		 /* offset of array of nodes in topo file */
    off_t Edge_offset;           /* offset of array of edges in topo file */
    off_t Line_offset;           /* offset of array of lines in topo file */
    off_t Area_offset;           /* offset of array of areas in topo file */
    off_t Isle_offset;           /* offset of array of isles in topo file */
    off_t Volume_offset;         /* offset of array of volumes in topo file */
    off_t Hole_offset;           /* offset of array of holes in topo file */

    /*** spatial index ***/
    int Spidx_built;		 /* set to 1 if spatial index is available */
    int Spidx_new;               /* set to 1 if new spatial index will be generated */

    struct gvfile spidx_fp;	 /* spatial index file pointer */

    char *spidx_node_fname;      /* node spatial index file name */

    off_t Node_spidx_offset;	 /* offset of nodes in sidx file */
    off_t Line_spidx_offset;     /* offset of lines in sidx file */
    off_t Area_spidx_offset;     /* offset of areas in sidx file */
    off_t Isle_spidx_offset;     /* offset of isles in sidx file */
    off_t Face_spidx_offset;     /* offset of faces in sidx file */
    off_t Volume_spidx_offset;   /* offset of volumes in sidx file */
    off_t Hole_spidx_offset;     /* offset of holes in sidx file */

    struct RTree *Node_spidx;     /* node spatial index */
    struct RTree *Line_spidx;     /* line spatial index */
    struct RTree *Area_spidx;     /* area spatial index */
    struct RTree *Isle_spidx;     /* isle spatial index */
    struct RTree *Face_spidx;     /* face spatial index */
    struct RTree *Volume_spidx;   /* volume spatial index */
    struct RTree *Hole_spidx;     /* hole spatial index */

    /*** category index ***/
    /* By default, category index is not updated */
    int update_cidx;		  /* update category index if vector is modified */

    int n_cidx;			  /* number of cat indexes (one for each field) */
    int a_cidx;			  /* allocated space for cat indexes */
    struct Cat_index *cidx;	  /* array of category indexes */
    int cidx_up_to_date;	  /* set to 1 when cidx is created and reset to 0 whenever any line is changed */

    off_t coor_size;		  /* size of coor file */
    long coor_mtime;		  /* time of last coor modification */

    /*** level 2 ***/
    /* update: list of lines and nodes updated (topo info for the line
       was changed) by last write/rewrite/delete operation.
       Lines/nodes in the list may be deleted (e.g. delete boundary:
       first added for delete area and then delete */
    int do_uplist;		  /* used internaly in diglib to know if list is maintained */

    int *uplines;		  /* array of updated lines */
    int alloc_uplines;		  /* allocated array */
    int n_uplines;		  /* number of updated lines */
    int *upnodes;		  /* array of updated nodes */
    int alloc_upnodes;		  /* allocated array */
    int n_upnodes;		  /* number of updated nodes */
};

/*! \brief Vector map info */
struct Map_info
{
    /* common info for all formats */
    int format;			/* format (native, ogr) */
    int temporary;		/* temporary file flag, not yet used */

    struct dblinks *dblnk;	/* info about db links */

    struct Plus_head plus;	/* topo file head info */

    /* graph-related section */
    int graph_line_type;	/* line type used to build the graph */
    dglGraph_s graph;		/* graph structure */
    dglSPCache_s spCache;	/* shortest path cache */
    double *edge_fcosts;	/* costs used for graph, (dglGetEdge()
				   is not supported for _DGL_V1) */
    double *edge_bcosts;
    double *node_costs;		/* node costs */
    int cost_multip;		/* edge and node costs
				   multiplicator */

    int open;			/* should be 0x5522AA22 (VECT_OPEN_CODE) if opened correctly
				   or        0x22AA2255 (VECT_CLOSED_CODE) if closed
				   anything else implies that structure has
				   never been initialized
				*/
    int mode;			/* Open mode - read (GV_MODE_READ),
				   write (GV_MODE_WRITE),
				   rw (GV_MODE_RW) */
    int level;			/* Topology level - 1, 2, (3) */
    int head_only;		/* only header is opened */
    int support_updated;	/* support files were updated */
    plus_t next_line;		/* for level 2 sequential reads */

    char *name;			/* for 4.0  just name, and mapset */
    char *mapset;               /* mapset name */
    /* location and gisdbase is usefull if changed (v.proj or external apps) */
    char *location;		/* location name */
    char *gisdbase;		/* gisdbase path */

    /* constraints for reading in lines  (not polys yet) */
    int Constraint_region_flag;
    int Constraint_type_flag;
    double Constraint_N;
    double Constraint_S;
    double Constraint_E;
    double Constraint_W;
    double Constraint_T;
    double Constraint_B;
    int Constraint_type;
    int proj;

    /* format specific */
    /* native */
    struct gvfile dig_fp;	/* dig file pointer */
    struct dig_head head;	/* coor file head */

    /* non native */
    struct Format_info fInfo;	/* format information */

    /* history file */
    FILE *hist_fp;

    /* temporary solution for sites - to be removed ?*/
    struct site_att *site_att;	/* array of attributes loaded from db */
    int n_site_att;		/* number of attributes in site_att array */
    int n_site_dbl;		/* number of double attributes for one site */
    int n_site_str;		/* number of string attributes for one site */
};

/*! \brief Node (topology) info */
struct P_node
{
    double x;			/* X coordinate */
    double y;			/* Y coordinate */
    double z;			/* Z coordinate */
    plus_t alloc_lines;         /* allocated space for lines */
    plus_t n_lines;		/* number of attached lines (size of
				   lines, angle). If 0, then is
				   degenerate node, for snappingi ???
				*/
    plus_t *lines;		/* list of connected lines */
    float *angles;		/* respected angles; angles for
				   lines/boundaries are in radians
				   between -PI and PI. Value for
				   points or lines with identical
				   points (degenerated) is set to
				   -9. */
};

/*! \brief Line (topology) info */
struct P_line
{
    plus_t N1;			/* start node */
    plus_t N2;			/* end node   */
    plus_t left;		/* area/isle number to left, negative
				   for isle area number for centroid,
				   negative for duplicate centroid
				*/
    plus_t right;		/* area/isle number to right, negative
				 * for isle */

    double N;			/* line bounding Box */
    double S;
    double E;
    double W;
    double T;			/* top */
    double B;			/* bottom */

    off_t offset;		/* offset in coor file for line */
    int type;                   /* line type */
};

/*! \brief Area (topology) info */
struct P_area
{
    double N;			/* area bounding Box */
    double S;
    double E;
    double W;
    double T;			/* top */
    double B;			/* bottom */
    plus_t n_lines;		/* number of boundary lines */
    plus_t alloc_lines;         /* allocated space */
    plus_t *lines;		/* list of boundary lines, negative
				   means direction N2 to N1, lines are
				   in clockwise order */

    /*********  Above this line is compatible with P_isle **********/
    plus_t centroid;		/* number of first centroid within area */

    plus_t n_isles;		/* number of islands inside */
    plus_t alloc_isles;
    plus_t *isles;		/* 1st generation interior islands */
};

/*! \brief Isle (topology) info */
struct P_isle
{
    double N;			/* isle bounding Box */
    double S;
    double E;
    double W;
    double T;			/* top */
    double B;			/* bottom */
    plus_t n_lines;		/* number of boundary lines */
    plus_t alloc_lines;
    plus_t *lines;		/* list of boundary lines, negative
				   means direction N2 to N1, lines are
				   in counter clockwise order */

    /*********  Above this line is compatible with P_area **********/
    plus_t area;		/* area it exists w/in, if any */
};

/*! \brief Line points (feature geometry info */
struct line_pnts
{
    double *x;                  /* pointer to 1st x coordinate */
    double *y;                  /* pointer to 1st y coordinate */
    double *z;                  /* pointer to 1st z coordinate */
    int n_points;               /* number of points */
    int alloc_points;           /* allocated space */
};

/*! \brief Line categories (feature category info */
struct line_cats
{
    int *field;			/* pointer to array of fields */
    int *cat;			/* pointer to array of categories */
    int n_cats;			/* number of vector categories attached to element */
    int alloc_cats;		/* allocated space */
};

/*! \brief Category list */
struct cat_list
{
    int field;			/* category field */
    int *min;			/* pointer to array of minimun values */
    int *max;			/* pointer to array of maximum values */
    int n_ranges;		/* number ranges */
    int alloc_ranges;		/* allocated space */
};

/*! \brief List of integers

  \todo To be moved to gislib?
*/
struct ilist
{
    int *value;			/* items */
    int n_values;		/* number of values */
    int alloc_values;		/* allocated space */
};

/*! \brief Vector array. Space allocated is size + 1. */
struct varray
{
    int size;			/* array size */
    int *c;			/* array where 'class' or new category
				   or something like that is stored */
};

/*! \brief Spatial index for use in modules. */
struct spatial_index
{
    struct RTree *si_tree;      /* pointer to the tree */
    char *name;                 /* sidx name */
};

#endif /* DIG___STRUCTS___ */
