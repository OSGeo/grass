/*!
  \file include/vect/dig_structs.h

  \brief Data structures for \ref vectorlib

  \author Written by Dave Gerdes (CERL)  5/1988
  \author Updated to GRASS 5.7 by Radim Blazek (2001)
  \author Updated to GRASS 7.0 by Markus Metz (2011)
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
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
#include <grass/dbmi.h>

#ifdef HAVE_OGR
#include <ogr_api.h>
#endif

#ifdef HAVE_POSTGRES
#include <libpq-fe.h>
#endif

/*!
  \brief plus_t size

  3.10 changes plus_t to int. This assumes that any reasonable
  machine will use 4 bytes to store an int. The diglib code is not
  guaranteed to work if plus_t is changed to a type that is larger
  than an int.
*/
typedef int plus_t;

/*!
  \brief Used by sites lib
*/
struct site_att
{
    /*!
      \brief Category number
    */
    int cat;
    /*!
      \brief Array of double attributes
    */
    double *dbl;
    /*!
      \brief Array of string attributes
    */
    char **str;
};

/*!
  \brief Bounding box
*/
struct bound_box
{
    /*!
      \brief North
    */
    double N;
    /*!
      \brief South
    */
    double S;
    /*!
      \brief East
    */
    double E;
    /*!
      \brief West
    */
    double W;
    /*!
      \brief Top
    */
    double T;
    /*!
      \brief Bottom
    */
    double B;
};

/*!
  \brief File definition
*/
struct gvfile
{
    /*!
      \brief File descriptor
    */
    FILE *file;
    /*!
      \brief Pointer to beginning of the file in the memory
    */
    char *start;
    /*!
      \brief Current position set by dig_seek()
    */
    char *current;
    /*!
      \brief End of file in the memory (pointer to first byte after)
    */
    char *end;
    /*!
      \brief Size of the file loaded to memory
    */
    off_t size;
    /*!
      \brief Allocated space
    */
    off_t alloc;
    /*!
      \brief Is file loaded?

      - 0 - not loaded
      - 1 - loaded
    */
    int loaded;
};

/*!
  \brief Layer (old: field) information
*/
struct field_info
{
    /*!
      \brief Layer number
    */
    int number;
    /*!
      \brief Layer name (optional)
    */
    char *name;
    /*!
      \brief Name of DB driver ('sqlite', 'dbf', ...)
    */
    char *driver;
    /*!
      brief Name of database
    */
    char *database;
    /*!
      \brief Name of DB table
    */
    char *table;
    /*!
      \brief Name of key column (usualy 'cat')
    */
    char *key;
};

/*!
  \brief Database links
*/
struct dblinks
{
    /*!
      \brief Pointer to the first field_info structure
    */
    struct field_info *field;
    /*!
      \brief Number of allocated slots
    */
    int alloc_fields;
    /*!
      \brief Number of available layers (old: fields)
    */
    int n_fields;
};

/*!
  \brief Portability info
  
  Set by V1_open_new() or V1_open_old()
*/
struct Port_info
{
    /*!
      \brief File byte order
    */
    int byte_order;
    /*!
      \brief Size of `off_t` data type
    */
    int off_t_size;

    /*!
      \brief Conversion matrices between file and native byte order (double)
    */
    unsigned char dbl_cnvrt[PORT_DOUBLE];
    /*!
      \brief Conversion matrices between file and native byte order (float)
    */
    unsigned char flt_cnvrt[PORT_FLOAT];
    /*!
      \brief Conversion matrices between file and native byte order (long)
    */
    unsigned char lng_cnvrt[PORT_LONG];
    /*!
      \brief Conversion matrices between file and native byte order (int)
    */
    unsigned char int_cnvrt[PORT_INT];
    /*!
      \brief Conversion matrices between file and native byte order (short)
    */
    unsigned char shrt_cnvrt[PORT_SHORT];
    /*!
      \brief Conversion matrices between file and native byte order (off_t)
    */
    unsigned char off_t_cnvrt[PORT_OFF_T];
    /*!
      \brief Quick reading flag for double
      
      Specify if native byte order of that type is the same
      as byte order of vector file (TRUE) or not (FALSE)
    */
    int dbl_quick;
    /*!
      \brief Quick reading flag for float
      
      Specify if native byte order of that type is the same
      as byte order of vector file (TRUE) or not (FALSE)
    */
    int flt_quick;
    /*!
      \brief Quick reading flag for long
      
      Specify if native byte order of that type is the same
      as byte order of vector file (TRUE) or not (FALSE)
    */
    int lng_quick;
    /*!
      \brief Quick reading flag for int
      
      Specify if native byte order of that type is the same
      as byte order of vector file (TRUE) or not (FALSE)
    */
    int int_quick;
    /*!
      \brief Quick reading flag for short
      
      Specify if native byte order of that type is the same
      as byte order of vector file (TRUE) or not (FALSE)
    */
    int shrt_quick;
    /*!
      \brief Quick reading flag for off_t
      
      Specify if native byte order of that type is the same
      as byte order of vector file (TRUE) or not (FALSE)
    */
    int off_t_quick;
};

/*!
  \brief List of dead lines in the file

  \todo Implement it

  The space can be reused, not yet used
*/
struct recycle
{
    char dummy;
};

/*!
  \brief Vector map header data

  Holds header data of vector map (see \ref vlibMap_info)
*/
struct dig_head
{
    /*!
      \brief Organization name
    */
    char *organization;
    /*!
      \brief Map date
    */
    char *date;
    /*!
      \brief User name
    */
    char *user_name;
    /*!
      \brief Map name
    */
    char *map_name;
    /*!
      \brief Source date
    */
    char *source_date;
    /*!
      \brief Original scale
    */
    long orig_scale;
    /*!
      \brief Comments
    */
    char *comment;
    int proj;			/* projection */

    /*!
      \brief Zone (UTM only)
    */
    int plani_zone;
    /*!
      \brief Threshold for digitization
    */
    double digit_thresh;

    /* Programmers should NOT touch any thing below here */
    /* Library takes care of everything for you          */

    /* coor elements */
    /*!
      \brief Backward compatibility info - major version
    */
    int Version_Major;
    /*!
      \brief Backward compatibility info - minor version
    */
    int Version_Minor;
    /*!
      \brief Backward compatibility info - back major version
    */
    int Back_Major;
    /*!
      \brief Backward compatibility info - back minor version
    */
    int Back_Minor;
    /*!
      \brief 2D/3D vector data

      - zero for 2D data
      - non-zero for 3D data
    */
    int with_z;

    /*!
      \brief Coor file size
    */
    off_t size;
    /*!
      \brief Coor header size
    */
    long head_size;

    /*!
      \brief Portability information
    */
    struct Port_info port;

    /*!
      \brief Offset of last read line
    */
    off_t last_offset;

    /*!
      \brief Recycle dead line

      \todo Not implemented yet
    */
    struct recycle *recycle;
};

/*!
  \brief Coor file info
*/
struct Coor_info
{
    /*!
      \brief Total size (in bytes)
    */
    off_t size;
    /*!
      \brief Time of last modification
    */
    long mtime;			
};

/*!
  \brief Data structure used for building pseudo-topology

  See Vect__build_sfa() (Format_info_ogr and Format_info_pg) for
  implementation issues.
*/
struct Format_info_offset
{
    /*!
      \brief Offset list

      Array where feature/part info is stored for each feature in
      GRASS. This is not used for GV_CENTROID. Because one feature may
      contain more elements (geometry collection also recursively),
      offset for one line may be stored in more records. First record
      is FID, next records are part indexes if necessary.

      Example 1:
      
      5. ring in 3. polygon in 7. feature (multipolygon) of geometry
      collection which has FID = 123 123 (feature 123: geometry
      colletion) 6 (7. feature in geometry collection: multiPolygon) 2
      (3. polygon) 4 (5. ring in the polygon)

      Example 2: geometry collection FID '1' containing one point, one
      linestring and one polygon

      \verbatim
      Offset:

      idx  offset note
      ----------------
      0	   1	  FID
      1	   0	  first part (point)

      2	   1	  FID
      3	   1	  second part (linestring)

      4	   1	  FID
      5	   2	  third part (polygon)
      6	   0	  first ring of polygon

      Topology:
      
      line idx
      -----------------
      1    0      point
      2    2      line
      3    4      boundary
      4    1      centroid read from topo (idx == FID)
      \endverbatim
    */
    int *array;
    /*!
      \brief Number of items in offset list
    */
    int array_num;
    /*!
      \brief Space allocated for offset list
    */
    int array_alloc;

};

/*!
  \brief Lines cache for reading feature (non-native formats)
*/
struct Format_info_cache {
    /*!
      \brief Lines array
      
      Some features requires more allocated lines (eg. polygon
      with more rings, multipoint, or geometrycollection)
    */
    struct line_pnts **lines;
    /*!
      \brief List of line types (GV_POINT, GV_LINE, ...)
    */
    int *lines_types;
    /*!
      \brief Number of allocated lines in cache
    */
    int lines_alloc;
    /*!
      \brief Number of lines which forms current feature
    */
    int lines_num;
    /*!
      \brief Next line to be read from cache
    */
    int lines_next;
    /*!
      \brief Feature id
    */
    long fid;
    /*!
      \brief Simple feature type (currently used only by PG format)
    */
    SF_FeatureType sf_type;
};

/*!
  \brief Non-native format info (OGR)

  \todo Structure size should not change depending on compilation I
  think, do it better
*/
struct Format_info_ogr
{
    /*!
      \brief OGR driver name
    */
    char *driver_name;
    /*!
      \brief OGR datasource name
    */
    char *dsn;
    /*!
      \brief OGR layer name
    */
    char *layer_name;
#ifdef HAVE_OGR
    /*!
      \brief Pointer to OGRDriver
    */
    OGRSFDriverH driver;
    /*!
      \brief Pointer to OGRDataSource
    */
    OGRDataSourceH ds;
    /*!
      \brief Pointer to OGRLayer
     */
    OGRLayerH layer;
#else
    void *driver;
    void *ds;
    void *layer;
#endif
    
    /*!
      \brief Open DB driver when writing attributes

      This driver is open by V2_open_new_ogr() and closed by
      V1_close_ogr().
    */
    dbDriver *dbdriver;
    
    /*!
      \brief Array of OGR DSN options
    */
    char **dsn_options;
    /*!
      \brief Array of OGR layer options
    */
    char **layer_options;
    
    /*!
      \brief Lines cache for reading feature
    */
    struct Format_info_cache cache;

    /*!
      \brief Cache to avoid repeated reading (level 2)

      NULL if no feature is in cache
    */
#ifdef HAVE_OGR
    OGRFeatureH feature_cache;
#else
    void *feature_cache;
#endif
  
    /*!
      \brief Offset list used for building pseudo-topology
    */
    struct Format_info_offset offset;
    
    /*!					      
      \brief Next line to be read

      Used by V2_read_next_line_ogr()
    */
    int next_line;
};

/*!
  \brief Non-native format info (PostGIS)
*/
struct Format_info_pg
{
    /*!
      \brief Connection info string
    */
    char    *conninfo;
    /*!
      \brief Database name (derived from conninfo)
    */
    char    *db_name;
    /*!
      \brief Schema name
    */
    char    *schema_name;
    /*!
      \brief Table name
    */
    char    *table_name;
    /*!
      \brief FID column
    */
    char    *fid_column;        
    /*!
      \brief Geometry column
    */
    char    *geom_column;
    /*!
      \brief Feature type
    */
    SF_FeatureType feature_type;
    /*!
      \brief Coordinates dimension
    */
    int      coor_dim;
    /*!
      \brief SRS ID
    */
    int      srid;

    /*!
      \brief Open DB driver when writing attributes

      This driver is open by V2_open_new_pg() and closed by
      V1_close_pg().
    */
    dbDriver *dbdriver;

    /*!
      \brief Start/Finish transaction
    */
    int       inTransaction;
#ifdef HAVE_POSTGRES
    /*!
      \brief PGconn object (generated by PQconnectdb)
    */
    PGconn   *conn;
    PGresult *res;
#else
    void     *conn;
    void     *res;
#endif
  
    /*!
      \brief Next line to be read for sequential access
    */
    int next_line;

    /*!
      \brief Lines cache for reading feature
    */
    struct Format_info_cache cache;
    
    /*!
      \brief Offset list used for building pseudo-topology
    */
    struct Format_info_offset offset;

    /* PostGIS topology support */
    /*!
      \brief TopoGeometry column (feature table)
    */
    char    *topogeom_column;
    /*!
      \brief Topology schema name and id
    */
    char    *toposchema_name;
    int      toposchema_id;
};

/*!
  \brief Non-native format info (currently only OGR is implemented)
*/
struct Format_info
{
    /*!
      \brief id?
    */
    int i;
    /*!
      \brief OGR info
    */
    struct Format_info_ogr ogr;
    /*!
      \brief PostGIS info
    */
    struct Format_info_pg  pg;
};

/*!
  \brief Category index
*/
struct Cat_index
{
    /*!
      \brief Field (layer) number
    */
    int field;			
    /*!
      \brief Number of items in cat array
    */
    int n_cats;			
    /*!
      \brief Allocated space in cat array
    */
    int a_cats;
    /*!
      \brief Array of cats (cat, type, lines/area)
    */
    int (*cat)[3];		
    /*!
      \brief Number of unique cats (not updated)
    */
    int n_ucats;
    /*!
      \brief Number of types in type
    */
    int n_types;		
    /*!
      \brief Number of elements for each type

      - GV_POINT
      - GV_LINE
      - GV_BOUNDARY
      - GV_CENTROID
      - GV_FACE
      - GV_KERNEL
      - GV_AREA
    */
    int type[7][2];		
    /*!
      \brief Offset of the beginning of this index in cidx file
    */
    off_t offset;
};

/*!
  \brief Basic topology-related info

  Holds basic topology-related information about vector map
*/
struct Plus_head
{
    /*!
      \brief Version info (major)
    */
    int Version_Major;
    /*!
      \brief Version info (minor)
    */
    int Version_Minor;
    /*!
      \brief Earliest version that can use this data format (major)
    */
    int Back_Major;		 
    /*!
      \brief Earliest version that can use this data format (minor)
    */
    int Back_Minor;
    /*!
      \brief Version codes for spatial index (major)
    */
    int spidx_Version_Major;
    /*!
      \brief Version codes for spatial index (minor)
    */
    int spidx_Version_Minor;
    /*!
      \brief Earliest version that can use this data format (major)
    */
    int spidx_Back_Major;
     /*!
       \brief Earliest version that can use this data format (minor)
    */
    int spidx_Back_Minor;

    /*!
      \brief Version codes for category index (major)
    */
    int cidx_Version_Major;
    /*!
      \brief Version codes for category index (minor)
    */
    int cidx_Version_Minor;
    /*!
      \brief Earliest version that can use this data format (major)
    */
    int cidx_Back_Major;
    /*!
      \brief Earliest version that can use this data format (minor)
    */
    int cidx_Back_Minor;

    /*!
      \brief 2D/3D vector data

      - WITH_Z
      - WITHOUT_Z
    */
    int with_z;
    /*!
      \brief 2D/3D spatial index

      - WITH_Z
      - WITHOUT_Z
    */
    int spidx_with_z;

    /*!
      \brief Offset size

      Because Plus_head is available to all releveant
      functions
    */
    int off_t_size;

    /*** file header size ***/
    
    /*!
      \brief Topo header size 
    */
    long head_size;
    /*!
      \brief Spatial index header size
    */
    long spidx_head_size;
    /*!
      \brief Category index header size
    */
    long cidx_head_size;

    /*!
      \brief Release memory occupied by support structures
      (topo, spatial, category)
    */
    int release_support;

    /*** portability info */

    /*!
      \brief Portability information
    */
    struct Port_info port;
    /*!
      \brief Portability information for spatial index
    */
    struct Port_info spidx_port;
    /*!
      \brief Portability information for category index
    */
    struct Port_info cidx_port;
    /*!
      \brief Access mode
      
      - GV_MODE_READ
      - GV_MODE_WRITE
      - GV_MODE_RW
    */
    int mode;

    /*!
      \brief Highest level of topology currently available

      - GV_BUILD_NONE
      - GV_BUILD_BASE
      - GV_BUILD_AREAS
      - GV_BUILD_ATTACH_ISLES
      - GV_BUILD_CENTROIDS
      - GV_BUILD_ALL
    */
    int built;
    /*!
      \brief Bounding box of features
    */
    struct bound_box box;

    /*** topology ***/
   /*!
     \brief Array of nodes
   */
    struct P_node **Node;
   /*!
     \brief Array of vector geometries
   */
    struct P_line **Line;
   /*!
     \brief Array of areas
   */
    struct P_area **Area;
    /*!
      \brief Array of isles
    */
    struct P_isle **Isle;
    
    /* add here P_FACE, P_VOLUME, P_HOLE */

    /*!
      \brief Current number of points
    */
    plus_t n_plines;
    /*!
      \brief Current number of lines
    */
    plus_t n_llines;
    /*!
      \brief Current number of boundaries
    */
    plus_t n_blines;
    /*!
      \brief Current number of centroids
    */
    plus_t n_clines;
    /*!
      \brief Current number of faces
    */
    plus_t n_flines;
    /*!
      \brief Current number of kernels
    */
    plus_t n_klines;
    /*!
      \brief Current number of volume faces
    */
    plus_t n_vfaces;
    /*!
      \brief Current number of hole faces
    */
    plus_t n_hfaces;
    
    /*!
      \brief Current number of topological features derived from vector
      geometries
    */
    /*!
      \brief Current number of nodes
    */
    plus_t n_nodes;		 
    /*!
      \brief Current number of edges
    */
    plus_t n_edges;	
    /*!
      \brief Current number of lines
    */
    plus_t n_lines;
    /*!
      \brief Current number of areas
    */
    plus_t n_areas;
    /*!
      \brief Current number of isles
    */
    plus_t n_isles;
    /*!
      \brief Current number of faces
    */
    plus_t n_faces;
    /*!
      \brief Current number of volumes
    */
    plus_t n_volumes;
    /*!
      \brief Current number of holes
    */
    plus_t n_holes;

   /*!
     \brief Number of allocated nodes

     i.e. array size - 1
   */
    plus_t alloc_nodes;
   /*!
     \brief Number of allocated edges

     i.e. array size - 1
   */
    plus_t alloc_edges;
   /*!
     \brief Number of allocated lines

     i.e. array size - 1
   */
    plus_t alloc_lines;
   /*!
     \brief Number of allocated areas

     i.e. array size - 1
   */
    plus_t alloc_areas;
   /*!
     \brief Number of allocated isles

     i.e. array size - 1
   */
    plus_t alloc_isles;
   /*!
     \brief Number of allocated faces

     i.e. array size - 1
   */
    plus_t alloc_faces;
   /*!
     \brief Number of allocated volumes

     i.e. array size - 1
   */
    plus_t alloc_volumes;
   /*!
     \brief Number of allocated holes

     i.e. array size - 1
   */
    plus_t alloc_holes;

    /*!
      \brief Offset of array of nodes in topo file
    */
    off_t Node_offset;
    /*!
      \brief Offset of array of edges in topo file
    */
    off_t Edge_offset;
    /*!
      \brief Offset of array of vector geometries in topo file
    */
    off_t Line_offset;
    /*!
      \brief Offset of array of areas in topo file
    */
    off_t Area_offset;
    /*!
      \brief Offset of array of isles in topo file
    */
    off_t Isle_offset;
    /*!
      \brief Offset of array of volumes in topo file
    */
    off_t Volume_offset;
    /*!
      \brief Offset of array of holes in topo file
    */
    off_t Hole_offset;

    /*** spatial index ***/
    /*!
      \brief Spatial index built?

      Set to 1 if spatial index is available
    */
    int Spidx_built;
    /*!
      \brief Build new spatial index

      Set to 1 if new spatial index will be generated
    */
    int Spidx_new;
    /*!
      \brief Build new spatial index in file

      Set to 1 to build new indices in file
    */
    int Spidx_file;

    /*!
      \brief Spatial index file pointer
    */
    struct gvfile spidx_fp;

    /*!
      \brief Offset of nodes in sidx file
    */
    off_t Node_spidx_offset;
    /*!
      \brief Offset of lines in sidx file
    */
    off_t Line_spidx_offset;
    /*!
      \brief Offset of areas in sidx file
    */
    off_t Area_spidx_offset;
    /*!
      \brief Offset of isles in sidx file
    */
    off_t Isle_spidx_offset;
    /*!
      \brief Offset of faces in sidx file
    */
    off_t Face_spidx_offset;
    /*!
      \brief Offset of volumes in sidx file
    */
    off_t Volume_spidx_offset;
    /*!
      \brief Offset of holes in sidx file
    */
    off_t Hole_spidx_offset;

    /*!
      \brief Node spatial index
    */
    struct RTree *Node_spidx;
    /*!
      \brief Line spatial index
    */
    struct RTree *Line_spidx;
    /*!
      \brief Area spatial index
    */
    struct RTree *Area_spidx;
    /*!
      \brief Isles spatial index
    */
    struct RTree *Isle_spidx;
    /*!
      \brief Faces spatial index
    */
    struct RTree *Face_spidx;
    /*!
      \brief Volumes spatial index
    */
    struct RTree *Volume_spidx;
    /*!
      \brief Holes spatial index
    */
    struct RTree *Hole_spidx;

    /*** category index ***/
    /*!
      \brief Update category index if vector is modified 

      By default, category index is not updated 
    */
    int update_cidx;

    /*!
      \brief Number of category indexes (one for each field/layer)
    */
    int n_cidx;
    /*!
      \brief Allocated space for category indexes
    */
    int a_cidx;
    /*!
      \brief Array of category indexes
    */
    struct Cat_index *cidx;
    /*!
      \brief Category index to be updated

      Set to 1 when cidx is created
      and reset to 0 whenever any line is changed
    */
    int cidx_up_to_date;

    /*!
      \brief Size of coor file
    */
    off_t coor_size;
    /*!
      \brief Time of last coor modification
    */
    long coor_mtime;

    /*** level 2 ***/
    /*!
      \brief List of updated lines/nodes

      Note: Vect_set_updated() must be called to maintain this list
    */
    struct {
	/*!
	  \brief Indicates if the list of updated features is maintained
	*/
	int do_uplist;
	
	/*!
	  \brief Array of updated lines
	  
	  List of lines and nodes updated (topo info for the line was
	  changed) by last write/rewrite/delete operation.
	  Lines/nodes in the list may be deleted (e.g. delete
	  boundary: first added for delete area and then delete
	*/
	int *uplines;
	/*!
	  \brief Array of updated lines - offset

	  Negative value for dead (deleted) lines - used by Vect_restore_line()
	*/
	off_t *uplines_offset;
	/*!
	  \brief Allocated array of lines
	*/
	int alloc_uplines;
	/*!
	  \brief Number of updated lines
	*/
	int n_uplines;
	/*!
	  \brief Array of updated nodes 
	*/
	int *upnodes;
	/*!
	  \brief Allocated array of nodes
	*/
	int alloc_upnodes;
	/*!
	  \brief number of updated nodes
	*/
	int n_upnodes;
    } uplist;
};

/*! \brief
  Vector map info

  Maintains all information about an individual open vector map. The
  structure must be passed to the most vector library routines. 
*/
struct Map_info
{
    /*** common info for all formats ***/

    /*!
      \brief Map format (native, ogr, postgis)
      
      - GV_FORMAT_NATIVE
      - GV_FORMAT_OGR
      - GV_FORMAT_OGR_DIRECT
      - GV_FORMAT_POSTGIS
    */
    int format;
    
    /*!
      \brief Temporary map flag
    */
    int temporary;

    /*!
      \brief Array of DB links
    */
    struct dblinks *dblnk;

    /*!
      \brief Topology info
    */
    struct Plus_head plus;

    /*!
      \brief Graph-related section - line type used to build the graph
     */
    int graph_line_type;
    /*!
      \brief Graph-related section - graph structure
    */
    dglGraph_s graph;
    /*!
      \brief Graph-related section - shortest path cache
    */
    dglSPCache_s spCache;
    /*!
      \brief Graph-related section - forward costs used for graph

      dglGetEdge() is not supported for _DGL_V1)
    */
    double *edge_fcosts;
    /*!
      \brief Graph-related section - backward costs used for graph
    */
    double *edge_bcosts;
    /*!
      \brief Graph-related section - node costs used for graph
    */
    double *node_costs;
    /*!
      \brief Graph-related section - edge and node costs multiplicator
    */
    int cost_multip;

    /*!
      \brief Open indicator

      Should be 0x5522AA22 (VECT_OPEN_CODE) if opened correctly
      or        0x22AA2255 (VECT_CLOSED_CODE) if closed

      Anything else implies that structure has never been initialized
    */
    int open;

    /* Open mode

       - read (GV_MODE_READ),
       - write (GV_MODE_WRITE),
       - rw (GV_MODE_RW)
    */
    int mode;

    /*!
      \brief Topology level
      
      - 1 (without topo)
      - 2 (with 2D topology)
      - 3 (with 3D topology) - not yet implemented
    */
    int level;

    /*!
      \brief Open only header

      Non-zero code to open only header of vector map
    */
    int head_only;

    /*!
      \brief Support files were updated

      Non-zero code to indicate that supoort file were updated
    */
    int support_updated;

    /*!
      \brief Sequential read (level 1) - see Vect_read_next_line()

      Note: Line id starts with 1
    */
    plus_t next_line;

    /*!
      \brief Map name (for 4.0)
    */
    char *name;
    /*!
      \brief Mapset name
    */
    char *mapset;
    /*!
      \brief Location name

      Note: location and gisdbase is useful if changed (v.proj or external apps)
    */
    char *location;
    /*!
      \brief GISDBASE path
    */
    char *gisdbase;

    /*!
      \brief Constraints for sequential feature access
    */
    struct {
	/*!
	  \brief Non-zero value to enable region constraint
	*/
	int region_flag;
        /*!
          \brief Region (bbox) constraint
        */
	struct bound_box box;
	/*!
	  \brief Non-zero value to enable feature type constraint
	*/
	int type_flag;
        /*!
          \brief Feature type constraint
        */
	int type;
	/*!
	  \brief Non-zero value to enable field constraint
	*/
	int field_flag;
        /*!
          \brief Field number constraint (see line_cats structure)
        */
	int field;
    } constraint;
    
    /*!
      \brief ???
    */
    int proj;

    /*!
      \brief History file
    */
    FILE *hist_fp;

    /*** format specific ***/

    /*!
      \brief GV file pointer (native format only)
    */
    struct gvfile dig_fp;
    /*!
      \brief Coor file header info (native format only)
    */
    struct dig_head head;

    /*!
      \brief Format info for non-native formats
    */
    struct Format_info fInfo;

    /* temporary solution for sites - to be removed ?*/

    /*!
      \brief Array of attributes loaded from db
      
      \todo To be removed?
    */
    struct site_att *site_att;
    /*!
      \brief Number of attributes in site_att array

      \todo To be removed?
    */
    int n_site_att;
    /*!
      \brief Number of double attributes for one site

      \todo To be removed
    */
    int n_site_dbl;	
    /*!
      \brief Number of string attributes for one site

      \todo To be removed?
    */
    int n_site_str;	
};

/*!
  \brief Topological feature - node
*/
struct P_node
{
    /*!
      \brief X coordinate
    */
    double x;
    /*!
      \brief Y coordinate
    */
    double y;
    /*!
      \brief Z coordinate (used only for 3D data)
    */
    double z;
    /*!
      \brief Allocated space for lines
    */
    plus_t alloc_lines;
    /*!
      \brief Number of attached lines (size of
      lines, angle)

      If 0, then is degenerate node, for snapping ???
    */
    plus_t n_lines;
    /*!
      \brief List of connected lines

      Line id can be positive (for lines which starts at the node) or
      negative (for lines which ends at the node).
    */
    plus_t *lines;
    /*!
      \brief List of angles of connected lines

      Angles for lines/boundaries are in radians between -PI and
      PI. Value for points or lines with identical points
      (degenerated) is set to -9. See dig_calc_begin_angle() and
      dig_calc_end_angle() for details.
    */
    float *angles;
};

/*! 
  \brief Line topology
*/
struct P_topo_l
{
    /*! 
      \brief Start node
    */
    plus_t N1;
    /*! 
      \brief End node
    */
    plus_t N2;
};

/*!
  \brief Boundary topology
*/
struct P_topo_b
{
    /*! 
      \brief Start node
    */
    plus_t N1;
    /*! 
      \brief End node
    */
    plus_t N2;
    /*! 
      \brief Area number to the left, negative for isle
    */
    plus_t left;
    /*! 
      \brief Area number to the right, negative for isle
    */
    plus_t right;
};

/*!
  \brief Centroid topology
*/
struct P_topo_c
{
    /*! 
      \brief Area number, negative for duplicate centroid
    */
    plus_t area;
};

/*! 
  \brief Face topology
*/
struct P_topo_f
{
    /* TODO */
    /*! 
      \brief Array of edges
    */
    plus_t E[3];
    /*! 
      \brief Volume number to the left, negative for hole
    */
    plus_t left;
    /*! 
      \brief Volume number to the right, negative for hole
    */
    plus_t right;
};

/*! 
  \brief Kernel topology
*/
struct P_topo_k
{
    /*! 
      \brief Volume number, negative for duplicate kernel
    */
    plus_t volume;
};

/*!
  \brief Vector geometry
*/
struct P_line
{
    /*!
      \brief Line type

      - GV_POINT
      - GV_LINE
      - GV_BOUNDARY
      - GV_CENTROID
      - GV_FACE
      - GV_KERNEL
    */
    char type;
    /*!
      \brief Offset in coor file for line

      OGR-links: offset array index
      PG-links: node/edge id
    */
    off_t offset;
    /*!
      \brief Topology info

      NULL for points
    */
    void *topo;
};

/*!
  \brief Area (topology) info
*/
struct P_area
{
    /*!
      \brief Number of boundary lines
    */
    plus_t n_lines;
    /*!
      \brief Allocated space for lines
    */
    plus_t alloc_lines;
    /*!
      \brief List of boundary lines

      - negative means direction N2 to N1
      - lines are in clockwise order
    */
    plus_t *lines;

    /*********  Above this line is compatible with P_isle **********/

    /*!
      \brief Number of first centroid within area
    */
    plus_t centroid;
    /*!
      \brief Number of islands inside
    */
    plus_t n_isles;
    /*!
      \brief Allocated space for isles
    */
    plus_t alloc_isles;
    /*!
      \brief 1st generation interior islands
    */
    plus_t *isles;
};

/*!
  \brief Isle (topology) info
*/
struct P_isle
{
    /*!
      \brief Number of boundary lines
    */
    plus_t n_lines;
    /*!
      \brief Allocated space for lines
    */
    plus_t alloc_lines;
    /*!
      \brief List of boundary lines

      - negative means direction N2 to N1
      - lines are in counter clockwise order
    */
    plus_t *lines;
 
    /*********  Above this line is compatible with P_area **********/
    
    /*!
      \brief Area it exists w/in, if any
    */
    plus_t area;
};

/*!
  \brief Feature geometry info - coordinates
*/
struct line_pnts
{
    /*!
      \brief Array of X coordinates
    */
    double *x;
    /*!
      \brief Array of Y coordinates
    */
    double *y;
    /*!
      \brief Array of Z coordinates
    */
    double *z;
    /*!
      \brief Number of points
    */
    int n_points;
    /*!
      \brief Allocated space for points
    */
    int alloc_points;
};

/*!
  \brief Feature category info
*/
struct line_cats
{
    /*!
      \brief Array of layers (fields)
    */
    int *field;
    /*!
      \brief Array of categories
    */
    int *cat;
    /*!
      \brief Number of categories attached to element
    */
    int n_cats;
    /*!
      \brief Allocated space for categories
    */
    int alloc_cats;
};

/*! \brief Category list */
struct cat_list
{
    /*!
      \brief Category layer (field)
    */
    int field;
    /*!
      \brief Array of minimum values
    */
    int *min;
    /*!
      \brief Array of maximum values
    */
    int *max;
    /*!
      \brief Number of ranges
    */
    int n_ranges;
    /*!
      \brief Allocated space for ranges
    */
    int alloc_ranges;
};

/*!
   \brief List of bounding boxes with id
*/
struct boxlist
{
    /*!
      \brief Array of ids
    */
    int *id;
    /*!
      \brief Array of bounding boxes
    */
    struct bound_box *box;
    /*!
      \brief flag to indicate whether bounding boxes should be added
    */
    int have_boxes;
    /*!
      \brief Number of items in the list
    */
    int n_values;
    /*!
      \brief Allocated space for items
    */
    int alloc_values;
};

/*!
  \brief Vector array

  Space allocated is size + 1
*/
struct varray
{
    /*!
      \brief Array size
    */
    int size;	
    /*!
      \brief Array

      Where 'class' or new category
      or something like that is stored
    */
    int *c;	
};

/*!
  \brief Spatial index info

  For use in modules
*/
struct spatial_index
{
    /*!
      \brief Pointer to the search tree (R*-Tree)
    */
    struct RTree *si_tree;
    /*!
      \brief Name of file to store the search tree
    */
    char *name;
};

#endif /* DIG___STRUCTS___ */
