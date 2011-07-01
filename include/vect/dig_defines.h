/*!
  \file include/vect/dig_defines.h

  Defines for diglib (part of vector library)
*/

/*! \brief Exit/print/return on error

  \todo To be moved to gislib
*/
#define GV_FATAL_EXIT    0
#define GV_FATAL_PRINT   1
#define GV_FATAL_RETURN  2

/*! \brief Vector directory layout, element names */
/*! \brief Name of vector directory */
#define GV_DIRECTORY    "vector"	
/*! \brief Format description, data location (OGR) */
#define GV_FRMT_ELEMENT "frmt"
/*! \brief Native format, coordinates */
#define GV_COOR_ELEMENT "coor"
/*! \brief Native format, header information */	
#define GV_HEAD_ELEMENT "head"
/*! \brief Native format, link to database */
#define GV_DBLN_ELEMENT "dbln"
/*! \brief Native format, history file */
#define GV_HIST_ELEMENT "hist"
/*! \brief Native format, topology file */
#define GV_TOPO_ELEMENT "topo"
/*! \brief Native format, spatial index */
#define GV_SIDX_ELEMENT "sidx"
/*! \brief Native format, category index */
#define GV_CIDX_ELEMENT "cidx"
/*! \brief External format (OGR), feature index */
#define GV_FIDX_ELEMENT "fidx"

/*! \brief Endian check

  \todo To be moved to gislib?
*/
#define ENDIAN_LITTLE 0
#define ENDIAN_BIG    1
#define ENDIAN_OTHER  2

/*! \brief Sizes of types used in portable format (different names used in
  Vlib/ and diglib/ for the same thing)

  Assumptions:
   - double =    8 byte IEEE
   -  float =    4 byte IEEE
   -   long =    4 byte int
   -  short =    2 byte int
  
  \todo To be moved to gislib?
*/
#define PORT_DOUBLE 8
#define PORT_FLOAT  4
#define PORT_LONG   4
#define PORT_INT    4
#define PORT_SHORT  2
#define PORT_CHAR   1
#define PORT_OFF_T  8

/*! \brief replace by PORT_* in Vlib later and remove :
  
  \todo To be removed ?
*/
#define DBL_SIZ  8
#define FLT_SIZ  4
#define LNG_SIZ  4
#define SHRT_SIZ 2

/*! \brief Limits for portable types

  \todo To be moved to gislib?
*/
#define PORT_DOUBLE_MAX 1.7976931348623157e+308
#define PORT_DOUBLE_MIN 2.2250738585072014e-308
#define PORT_FLOAT_MAX  3.40282347e+38F
#define PORT_FLOAT_MIN  1.17549435e-38F
#define PORT_LONG_MAX   2147483647L
#define PORT_LONG_MIN   (-2147483647L)
#define PORT_INT_MAX    2147483647
#define PORT_INT_MIN    (-2147483647)
#define PORT_SHORT_MAX  32767
#define PORT_SHORT_MIN  (-32768)
#define PORT_CHAR_MAX   127
#define PORT_CHAR_MIN   (-128)

/*! \brief Geometry data formats supported by lib
  Don't change GV_FORMAT_* values, this order is hardcoded in lib
*/
/*! \brief GRASS native format */
#define GV_FORMAT_NATIVE     0
/*! \brief OGR format (for layers linked via v.external) */
#define GV_FORMAT_OGR        1
/*! \brief OGR format (direct access) */
#define GV_FORMAT_OGR_DIRECT 2

/*! \brief One table linked to vector map */
#define GV_1TABLE  0
/*! \brief More tables linked to vector map */
#define GV_MTABLE  1		

/*! \brief Read-only vector map open mode */
#define GV_MODE_READ  0
/*! \brief Write vector map open mode */
#define GV_MODE_WRITE 1
/*! \brief Read-write vector map open mode */
#define GV_MODE_RW    2

/*! \brief Vector map open code */
#define VECT_OPEN_CODE   0x5522AA22
/*! \brief Vector map close code */
#define VECT_CLOSED_CODE 0x22AA2255

/*! \brief Vector level

  - 1 without topology
  - 2 topology
  - 3 ?
*/
#define LEVEL_1  1
#define LEVEL_2  2
#define LEVEL_3  3

/*! \brief Topology level details */
/*! \brief Nothing to build */
#define GV_BUILD_NONE         0
/*! \brief Basic level */
#define GV_BUILD_BASE         1
/*! \brief Build areas */
#define GV_BUILD_AREAS        2
/*! \brief Attach islands to areas */
#define GV_BUILD_ATTACH_ISLES 3
/*! \brief Assign centroids to areas */
#define GV_BUILD_CENTROIDS    4
#define GV_BUILD_ALL          GV_BUILD_CENTROIDS

/*! \brief Check if vector map is open */
#define VECT_OPEN(Map)   (Map->open == VECT_OPEN_CODE)

/*! \brief Memory mode */
#define GV_MEMORY_ALWAYS 1
#define GV_MEMORY_NEVER  2
#define GV_MEMORY_AUTO   3

/*! \brief Coordinates file head size */
#define GV_COOR_HEAD_SIZE 14

#define GRASS_V_VERSION       "5.0"
/*! \brief The latest versions of files known by current version of
    the library. Used for new files */
#define GV_COOR_VER_MAJOR  5
#define GV_COOR_VER_MINOR  1
#define GV_TOPO_VER_MAJOR  5
#define GV_TOPO_VER_MINOR  1
#define GV_SIDX_VER_MAJOR  5
#define GV_SIDX_VER_MINOR  1
#define GV_CIDX_VER_MAJOR  5
#define GV_CIDX_VER_MINOR  0

/*! \brief The oldest versions of the library, which are capable to
    read the files created by the current version */
#define GV_COOR_EARLIEST_MAJOR  5
#define GV_COOR_EARLIEST_MINOR	1
#define GV_TOPO_EARLIEST_MAJOR  5
#define GV_TOPO_EARLIEST_MINOR	1
#define GV_SIDX_EARLIEST_MAJOR  5
#define GV_SIDX_EARLIEST_MINOR	1
#define GV_CIDX_EARLIEST_MAJOR  5
#define GV_CIDX_EARLIEST_MINOR	0

/*! \brief 2D/3D vector data */
#define WITHOUT_Z	0
#define WITH_Z		1

/*! \brief Side indicator left/right */
#define GV_LEFT	 1
#define GV_RIGHT 2

/*! \brief Direction indicator forward/backward */
#define GV_FORWARD  1
#define GV_BACKWARD 2

/*! \brief Types used in memory on run time (may change) */
#define GV_POINT      0x01
#define GV_LINE	      0x02
#define GV_BOUNDARY   0x04
#define GV_CENTROID   0x08
#define GV_FACE       0x10
#define GV_KERNEL     0x20
#define GV_AREA	      0x40
#define GV_VOLUME     0x80

#define GV_POINTS (GV_POINT | GV_CENTROID )
#define GV_LINES (GV_LINE | GV_BOUNDARY )

/*! \types Used in store like 'coor' file or postgis type column (must not change) */
#define GV_STORE_POINT    1
#define GV_STORE_LINE     2
#define GV_STORE_BOUNDARY 3
#define GV_STORE_CENTROID 4
#define GV_STORE_FACE     5
#define GV_STORE_KERNEL   6
#define GV_STORE_AREA     7	/* used in category index file */
#define GV_STORE_VOLUME   8	/* used in category index file */

/*! \brief Overlay operators */
#define GV_ON_AND     "AND"	/* intersect */
#define GV_ON_OVERLAP "OVERLAP"

enum overlay_operator
{
    GV_O_AND,
    GV_O_OVERLAP
};

typedef enum overlay_operator OVERLAY_OPERATOR;

/*! \brief Maximum number of categories for one element */
#define GV_NCATS_MAX PORT_INT_MAX
/*! \brief Maximum field */
#define GV_FIELD_MAX PORT_INT_MAX
/*! \brief Maximum category value */
#define GV_CAT_MAX   PORT_INT_MAX

/*! \brief GRASS ASCII vector format */
#define GV_ASCII_FORMAT_POINT 0
#define GV_ASCII_FORMAT_STD   1
#define GV_ASCII_FORMAT_WKT   2

/*! \brief Simple feature types */
#define SF_POINT      0x01
#define SF_LINE	      0x02
#define SF_LINESTRING 0x04
#define SF_LINEARRING 0x08
#define SF_POLYGON    0x10

#define HEADSTR	50
