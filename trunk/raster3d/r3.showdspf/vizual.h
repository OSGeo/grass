#include "vizual2.h"
#include <GL/gl.h>

#include <grass/gis.h>

#define WITHIN(a,x,b)   (((a) <= (x) && (x) <= (b)) ? 1 : 0)

#define INSIDE  0
#define OUTSIDE 1

extern file_info Headfax;	/* contains info about data itself */

extern file_info G3header;	/* contains info about data itself */

extern int G_sign;
extern int X_sign;

extern long D_offset;		/*offset to data in grid3 file */

struct poly_info
{
    int vnum;			/* the number of vertices in this polygon (= number of sides ) */
    double data[18];
    double verts[18];
};

struct color_entry
{
    float data;
    short color[3];
};


struct dspec
{
    int Thresh;
    int t[MAXTHRESH];		/* an array of index numbers */
    int nt;			/* number of indexes chosen (cumulative) */
    int xrot, yrot, zrot;	/* angle in degrees */
    int Xrot, Yrot, Zrot;	/* indicates if do autorotate */
    float xscale, yscale, zscale;	/* scaling factor for each dimension */
    float ztrans;
    int B[3];			/* sets the minimum dim to be displayed along this axis */
    /* the default is 0 */
    int E[3];			/* sets the maximum dim to be displayed along this axis */
    /* the default is xdim, ydim or zdim */
    float Xtran, Ytran, Ztran;	/* translation of object */
    int c_flag;			/* reset flag */
    int Swap_buf;

    int low, hi;		/* outside threshold indexes */
    int in_out;			/* fill contours between thresholds or outside */
    /* INSIDE 0  OUTSIDE 1 */
    /* Light model options */
    float Specular;
    int plane;			/* which plane we are looking at */
    float p[6][3][3];		/*using the bounding box vertices for plane normals */
    cmndln_info threshes[2];
    struct color_entry ctable[101];
    FILE *cfile;
    int grid;
};

/*
   GLOBAL struct dspec  D_spec;
 */

/*
 **  Structure to support drawing end caps
 */
struct Cap
{
    float *D_buff;		/* 2 dim data buffer */
    int reverse;		/* is it a mirror image? if so, polygons
				 **  should be drawn counter-clockwise */
    /* 1 means do reverse */
    int minx, miny;		/* sets the minimum dim to be displayed along this axis */
    /* the default is 0 */
    int maxx, maxy;		/* sets the maximum dim to be displayed along this axis */
    /* the default is Rows, Cols  */
    int z;			/* this is the axis that is a constant */
    int side;			/* which side 0 - 5 */
    int Cols, Rows;		/* Dims of current data in buffer */
};



/*
   struct Nodes {
   int used;
   float x[2];
   float y[2];
   };
 */
#define X 0
#define Y 1
#define Z 2

#define DRAW_BBOX  1
#define DRAW_ISO   2
#define DRAW_SOLID 4
#define DRAW_CAP  8		/* needs top half w/ side flags set */

    /* i.e.  for side two use:           ((1 << 2) << 16 )  | DRAW_CAP */
    /*       for side three use:         ((1 << 3) << 16 )  | DRAW_CAP */
    /*       for side two and three use: (((1<<2)|(1<<3)) << 16 ) | DRAW_CAP */

/*
   extern short carray[129][3];
   extern int  carray_max;
 */
