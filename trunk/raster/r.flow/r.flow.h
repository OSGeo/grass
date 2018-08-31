
/****************************************************************************
**  Written by Maros Zlocha, Jaroslav Hofierka, Helena Mitasova, Winter 1992
**  Comenius University, Bratislava, Slovakia and
**  US Army Construction Engineering Research Laboratories, Champaign, IL,USA.
**  Copyright Maros Zlocha, Jaroslav Hofierka, Helena Mitasova, USA-CERL  1992
**  Drastically re-engineered by Joshua Caplan, Spring, 1994
**  Continued improvements by Mark Ruesink, Fall, 1995
*****************************************************************************/

/*
 * changes from version 6: lg matrix changed to single cell row buffer
 *      x, y components of point structure are now exact coordinates
 *      r, c reflect file structure (i.e. r = distance in ns-res units from N) 
 * changes from version 7: next_point changed to eliminate arctangent
 *      theta and other angles kept in degrees instead of radians
 * changes from version 8: tangents replaced by lookup table
 *      bounding box re-introduced to reduce number of truncations
 * changes from version 9: source code split into separate modules
 *      aspect optionally computed internally (aspin no longer required)
 *      aspect optionally computed on the fly, eliminating o matrix
 *      resolution no longer used to compute distances (allows lat/long proj)
 *      downslope flag now the default, new options "bound", "-z", "-q" added
 *      diagnostic/verbose output --> stderr; debugging output --> stdout
 *      epsilon introduced to defeat quantization errors
 * changes from version 10: segmentation "-M" added, "-z" renamed to "-3"
 *      further modularization and abstraction
 * changes from version 11: replaced function pointers get, put, ... by macros
 * changes from version 12: updated to read FP data, writing of lg file broken
 * changes from version 13: offset option added, fixed output of FP lg
 */

#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/segment.h>
#include <grass/vector.h>
#include <grass/bitmap.h>

#ifndef hypot
#define hypot(x,y)	(sqrt((x)*(x)+(y)*(y)))
#endif
#define ROUND(x)	(int) ((x) + 0.5)

#define D_PI	180.
#define D2_PI	360.
#define DEG2RAD	M_D2R
#define UNDEF	365.		/* change to undefined when available */
#define UNDEFZ	0.		/* change to undefined when available */
#define HORIZ	1		/* magic        */
#define VERT	0		/*      numbers */


typedef struct
{
    char *elevin;		/* name of input elevation file         */
    char *aspin;		/* name of input aspect file            */
    char *barin;		/* name of barrier input file           */
    char *flout;		/* name of output flowline file         */
    char *lgout;		/* name of output length file           */
    char *dsout;		/* name of output density file          */
    int skip;			/* cells between flowlines output       */
    int bound;			/* constant bound on path length        */
    /*    double  offset;              magnitude of random grid offset      */
    char up;			/* direction to compute lines           */
    char l3d;			/* three-dimensional length             */
    char mem;			/* always recompute aspect              */
    char seg;			/* use segmented arrays                 */
}
params;

typedef struct
{
    DCELL **buf;		/* internal row storage                 */
    SEGMENT *seg;		/* state for segment library            */
    int sfd;			/* file descriptor for segment file     */
    int row_offset,		/* border widths of buf (for            */
      col_offset;		/*      extrapolating border data)      */
    char *name;			/* for error messages                   */
}
layer;

/******************************* GLOBALS ********************************/

extern struct Cell_head region;
extern struct Map_info fl;
extern struct BM *bitbar;
extern int lgfd;
extern char string[1024];
extern layer el, as, ds;
extern double *ew_dist;
extern double *epsilon[2];

extern params parm;

