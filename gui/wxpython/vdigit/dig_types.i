/* extracted from include/vect/dig_defines.h */

#define GV_POINT      0x01
#define GV_LINE       0x02
#define GV_BOUNDARY   0x04
#define GV_CENTROID   0x08
#define GV_FACE       0x10
#define GV_KERNEL     0x20
#define GV_AREA       0x40
#define GV_VOLUME     0x80

#define GV_POINTS (GV_POINT | GV_CENTROID )
#define GV_LINES (GV_LINE | GV_BOUNDARY )

#define PORT_DOUBLE_MAX 1.7976931348623157e+308

/* extracted from vector/v.edit/lib/vedit.h */

#define NO_SNAP    0 /* snapping disabled */
#define SNAP       1 /* snapping enabled for nodes */
#define SNAPVERTEX 2 /* snapping enabled for vertex also */

#define QUERY_UNKNOWN -1
#define QUERY_LENGTH   0 /* select by line length */
#define QUERY_DANGLE   1 /* select dangles */
