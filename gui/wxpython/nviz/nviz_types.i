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

/* extracted from gui/wxpython/nviz/nviz.h */

#define VIEW_DEFAULT_POS_X 0.85
#define VIEW_DEFAULT_POS_Y 0.85
#define VIEW_DEFAULT_PERSP 40.0
#define VIEW_DEFAULT_TWIST 0.0
#define VIEW_DEFAULT_ZEXAG 1.0

/* extracted from include/gsurf.h */
#define DM_GOURAUD   0x00000100
#define DM_FLAT      0x00000200

#define DM_FRINGE    0x00000010

#define DM_WIRE      0x00000001
#define DM_COL_WIRE  0x00000002
#define DM_POLY      0x00000004
#define DM_WIRE_POLY 0x00000008

#define DM_GRID_WIRE 0x00000400
#define DM_GRID_SURF 0x00000800

/* site markers */
#define ST_X          1
#define ST_BOX        2
#define ST_SPHERE     3
#define ST_CUBE       4
#define ST_DIAMOND    5
#define ST_DEC_TREE   6
#define ST_CON_TREE   7
#define ST_ASTER      8
#define ST_GYRO       9
#define ST_HISTOGRAM  10

#define MAX_ISOSURFS   12
#define MAX_SLICES     12

/* extracted from include/nviz.h */
/* quick draw mode */
#define DRAW_QUICK_SURFACE 0x01
#define DRAW_QUICK_VLINES  0x02
#define DRAW_QUICK_VPOINTS 0x04
#define DRAW_QUICK_VOLUME  0x08
