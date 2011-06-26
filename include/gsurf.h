/*!
   \file include/gsurf.h

   \brief OGSF library - main header

   (C) 1999-2008, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Bill Brown USACERL (January 1993)
 */

#ifndef _GSURF_H
#define _GSURF_H

#define GS_UNIT_SIZE 1000.

#define BETWEEN(x, a, b) (((x) > (a) && (x) < (b)) || ((x) > (b) && (x) < (a)))
#define GS_NEAR_EQUAL(x, y, ratio) ((x) == (y) || ((x) == 0.0? \
            GS_BETWEEN((x), (y)+(y)*(ratio), (y)-(y)*(ratio)):\
            GS_BETWEEN((y), (x)+(x)*(ratio), (x)-(x)*(ratio))))

/* current maximums */
#define MAX_SURFS      12
#define MAX_VECTS      50
#define MAX_SITES      50
#define MAX_VOLS       12	/* should match MAX_VOL_FILES below ? */
#define MAX_DSP        12
#define MAX_ATTS        7
#define MAX_LIGHTS      3
#define MAX_CPLANES     6
#define MAX_ISOSURFS   12
#define MAX_SLICES     12

/* for gvl_file.c */
#define MAX_VOL_SLICES         4
#define MAX_VOL_FILES        100

/* surface display modes */
#define DM_GOURAUD   0x00000100
#define DM_FLAT      0x00000200	/* defined for symmetry */

#define DM_FRINGE    0x00000010

#define DM_WIRE      0x00000001
#define DM_COL_WIRE  0x00000002
#define DM_POLY      0x00000004
#define DM_WIRE_POLY 0x00000008

#define DM_GRID_WIRE 0x00000400
#define DM_GRID_SURF 0x00000800

#define WC_COLOR_ATT 0xFF000000

#define IFLAG unsigned int

/* surface attribute ***descriptors***  */
#define ATT_NORM      0		/* library use only */
#define ATT_TOPO      1
#define ATT_COLOR     2
#define ATT_MASK      3
#define ATT_TRANSP    4
#define ATT_SHINE     5
#define ATT_EMIT      6
#define LEGAL_ATT(a) (a >= 0 && a < MAX_ATTS)

/* surface attribute **sources**  */
#define NOTSET_ATT   0
#define MAP_ATT      1
#define CONST_ATT    2
#define FUNC_ATT     3
#define LEGAL_SRC(s) (s==NOTSET_ATT||s==MAP_ATT||s==CONST_ATT||s==FUNC_ATT)

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

/* Buffer modes */
#define GSD_FRONT 1
#define GSD_BACK  2
#define GSD_BOTH  3

/* fence colormodes */
#define FC_OFF           0
#define FC_ABOVE         1
#define FC_BELOW         2
#define FC_BLEND         3
#define FC_GREY          4

/* legend types */
#define LT_DISCRETE      0x00000100
#define LT_CONTINUOUS    0x00000200

#define LT_LIST          0x00000010
/* list automatically discrete */

#define LT_RANGE_LOWSET  0x00000001
#define LT_RANGE_HISET   0x00000002
#define LT_RANGE_LOW_HI  0x00000003
#define LT_INVERTED      0x00000008

#define LT_SHOW_VALS     0x00001000
#define LT_SHOW_LABELS   0x00002000

/* types of volume files */
#define VOL_FTYPE_G3D        0

/* types of volume values */
#define VOL_DTYPE_FLOAT     0
#define VOL_DTYPE_DOUBLE    1

#endif /* _GSURF_H */
