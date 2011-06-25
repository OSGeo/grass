/*!
  \file include/gstypes.c

  \brief OGSF header file (structures)

  This program is free software under the GNU General
  Public License (>=v2). Read the file COPYING that
  comes with GRASS for details.
  
  \author Original author Bill Brown, USACERL (January 1993)
  \author Thematic mapping enabled by Martin Landa <landa.martin gmail.com) (06/2011)

  (C) 2011 by the GRASS Development Team
*/

#ifndef _GSTYPES_H
#define _GSTYPES_H

#include <grass/config.h>
#include <grass/gsurf.h>
#include <grass/bitmap.h>
#if defined(OPENGL_X11) || defined(OPENGL_WINDOWS)
#include <GL/gl.h>
#endif
#ifdef OPENGL_AQUA
#include <OpenGL/gl.h>
#endif

/*#define TRACE_FUNCS */
/*#define DEBUG */

#define X 0
#define Y 1
#define Z 2
#define W 3
#define FROM 0
#define TO 1

/* colormodes */
#define CM_COLOR               0
#define CM_EMISSION            1
#define CM_AMBIENT             2
#define CM_DIFFUSE             3
#define CM_SPECULAR            4
#define CM_AD                  5
#define CM_NULL                6

#define CM_WIRE CM_COLOR

#define NULL_COLOR 0xFFFFFF

/* attribute sizes - NOT YET USED */
#define GS_CHAR8      char
#define GS_SHORT16   short
#define GS_INT32       int

/* attribute ***types*** */
#define ATTY_NULL       32	/* internal use only */
#define ATTY_MASK       16	/* can't use this one for numbytes */
#define ATTY_FLOAT       8	/* can't use this one for numbytes */
#define ATTY_INT         4
#define ATTY_SHORT       2
#define ATTY_CHAR        1
#define ATTY_ANY        63	/* internal use only */
#define LEGAL_TYPE(t)    \
(t==ATTY_MASK || t==ATTY_FLOAT || t==ATTY_INT || t==ATTY_SHORT || t==ATTY_CHAR)

#define MAXDIMS 4

#define FUDGE(gs) ((gs->zmax_nz - gs->zmin_nz)/500.)
#define DOT3( a, b )    ( (a)[X]*(b)[X] + (a)[Y]*(b)[Y] + (a)[Z]*(b)[Z] )

/* changed flags for datasets */
#define CF_NOT_CHANGED 0x000000
#define CF_COLOR_PACKED 0x000001
#define CF_USR_CHANGED 0x000010
#define CF_CHARSCALED  0x000100

#define MAX_TF 6

#define MASK_TL 0x10000000
#define MASK_TR 0x01000000
#define MASK_BR 0x00100000
#define MASK_BL 0x00010000
#define MASK_NPTS 0x00000007

#define OGSF_POINT 1
#define OGSF_LINE  2
#define OGSF_POLYGON 3

#define RED_MASK 0x000000FF
#define GRN_MASK 0x0000FF00
#define BLU_MASK 0x00FF0000

typedef float Point4[4];
typedef float Point3[3];
typedef float Point2[2];

typedef struct
{
    float *fb;
    int *ib;
    short *sb;
    unsigned char *cb;
    struct BM *bm;
    struct BM *nm;		/* null mask: set = null */
    float (*tfunc) (float, int);
    float k;
} typbuff;

typedef struct
{				/* use hash table? */
    int n_elem;			/* if n_elem == 256, index == NULL */
    char *index;
    int *value;
} table256;

typedef struct
{				/* applied thusly: offset, mult, if(use_lookup) lookup */
    float offset;
    float mult;
    int use_lookup;
    table256 lookup;
} transform;

/* move this to dataset file? */
typedef struct
{
    int data_id;
    int dims[MAXDIMS];
    int ndims;
    int numbytes;
    char *unique_name;
    typbuff databuff;
    IFLAG changed;
    int need_reload;
} dataset;

/* maybe add transformation matrix? */
typedef struct
{
    IFLAG att_src;		/* NOTSET_ATT, MAP_ATT, CONST_ATT, FUNC_ATT */
    IFLAG att_type;		/* ATTY_INT, ATTY_SHORT, ATTY_CHAR, or ATTY_FLOAT */
    int hdata;			/* handle to dataset */
    int (*user_func) ();
    float constant;
    int *lookup;		/* TODO: use transform instead */
    float min_nz, max_nz, range_nz;
    float default_null;
} gsurf_att;

typedef struct g_surf
{
    int gsurf_id;
    int cols, rows;
    gsurf_att att[MAX_ATTS];	/* mask, topo, color, etc. */
    IFLAG draw_mode;		/*DM_GOURAUD | DM_FRINGE | DM_POLY, DM_WIRE, DM_WIRE_POLY */
    long wire_color;		/* 0xBBGGRR or WC_COLOR_ATT */
    double ox, oy;		/* real world origin (i.e., SW corner) */
    double xres, yres;
    float z_exag;
    float x_trans, y_trans, z_trans;
    float xmin, xmax, ymin, ymax, zmin, zmax, zminmasked;
    float xrange, yrange, zrange;
    float zmin_nz, zmax_nz, zrange_nz;
    int x_mod, y_mod, x_modw, y_modw;	/*cells per viewcell, per wire viewcell */
    int nz_topo, nz_color;	/* no zero flags */
    int mask_needupdate, norm_needupdate;
    unsigned long *norms;
    struct BM *curmask;
    struct g_surf *next;
    void *clientdata;
} geosurf;

/* maybe put attribute info here instead of in geovect - allow a single
   vector file to have multiple attributes ?   Cached lines should
   usually be stored as 2d, since they may be draped on multiple
   surfaces & Z will vary depending upon surface. */

/* Struct for vector feature displaying attributes */
typedef struct g_vect_style
{
    int color; 		 /* Line color */
    int symbol;		 /* Point symbol/line type */
    float size;		 /* Symbol size. Unset for lines. */
    int width;		 /* Line width. Also used for lines forming symbols i.e. X */
    
    /*TODO:fill;	 Area fill pattern */
    /*TODO:falpha;	 Area fill transparency */
    /*TODO:lalpha;	 Line/boundary/point transparency */
    /*TODO:struct *orientation;  Symbol orientation */
    struct g_vect_style *next; /* Point to next gvstyle struct if single point has multiple styles.
				  In such case feature with next style should be shifted. */
} gvstyle;

/* Struct for vector map (thematic mapping) */
typedef struct g_vect_style_thematic
{
    int layer;
    
    char *color_column;  
    char *symbol_column; 
    char *size_column;
    char *width_column;
} gvstyle_thematic;

/* Line instance */
typedef struct g_line
{
    int type;
    float norm[3];
    int dims, npts;
    Point3 *p3;
    Point2 *p2;
    
    struct line_cats *cats;	/* Store information about all layers/cats for thematic display */
    gvstyle *style;	/* Line instance look&feel */
    signed char highlighted; /* >0 Feature is highlighted */
    
    struct g_line *next;
} geoline;

/* Vector map (lines) */
typedef struct g_vect
{
    int gvect_id;
    int use_mem, n_lines;
    int drape_surf_id[MAX_SURFS];	/* if you want 'em flat, define the surface */
    int flat_val;
    int n_surfs;
    char *filename;
    float x_trans, y_trans, z_trans;
    /* also maybe center & rotate? */
    geoline *lines;
    geoline *fastlines;
    int (*bgn_read) (), (*end_read) (), (*nxt_line) ();
    struct g_vect *next;
    void *clientdata;

    int thematic_layer;		/* Layer number to use for thematic mapping. <0 == no thematic mapping; 
				   0 == thematic mapping is unset but initialized; 
				   >0 use specified layer */
    gvstyle *style;	/* Vector default look&feel */
    gvstyle *hstyle;	/* IMHO highlight should be per layer basis. */
} geovect;

/* Point instance */
typedef struct g_point
{
    int dims;
    Point3 p3;
    
    struct line_cats *cats;	/* Store information about all layers/cats for thematic display */
    gvstyle *style;
    signed char highlighted;    /* >0 Feature is highlighted */

    struct g_point *next;
} geopoint;

/* Vector map (points) */
typedef struct g_site
{
    int gsite_id;
    int drape_surf_id[MAX_SURFS];	/* ditto */
    int n_surfs, n_sites;
    int use_z, use_mem;
    int has_z;		/* set when file loaded */

    char *filename;
    transform attr_trans;
    float x_trans, y_trans, z_trans;
    geopoint *points;
    int (*bgn_read) (), (*end_read) (), (*nxt_site) ();
    struct g_site *next;
    void *clientdata;
    
    gvstyle_thematic *tstyle;  /* thematic mapping */
    gvstyle *style; 	       /* points default look&feel */
    gvstyle *hstyle;	       /* IMHO highlight should be per layer basis */
} geosite;

typedef struct
{
    int data_id;		/* id */
    IFLAG file_type;		/* file type */
    unsigned int count;		/* number of referencies to this file */
    char *file_name;		/* file name */

    IFLAG data_type;
    void *map;			/* pointer to volume file descriptor */
    double min, max;		/* minimum, maximum value in file */

    IFLAG status;		/* current status */
    IFLAG mode;			/* current read mode */

    void *buff;			/* data buffer */
} geovol_file;

typedef struct
{
    IFLAG att_src;

    int hfile;
    int (*user_func) ();
    float constant;

    void *att_data;
    int changed;
} geovol_isosurf_att;

typedef struct
{
    int inout_mode;
    geovol_isosurf_att att[MAX_ATTS];

    int data_desc;
    unsigned char *data;
} geovol_isosurf;

typedef struct
{
    int dir;
    float x1, x2, y1, y2, z1, z2;
    unsigned char *data;
    int changed;

    int mode, transp;
} geovol_slice;

typedef struct g_vol
{
    int gvol_id;
    struct g_vol *next;

    int hfile;
    int cols, rows, depths;
    double ox, oy, oz;
    double xres, yres, zres;
    double xmin, xmax, ymin, ymax, zmin, zmax;
    double xrange, yrange, zrange;
    float x_trans, y_trans, z_trans;

    int n_isosurfs;
    geovol_isosurf *isosurf[MAX_ISOSURFS];
    int isosurf_x_mod, isosurf_y_mod, isosurf_z_mod;
    IFLAG isosurf_draw_mode;

    int n_slices;
    geovol_slice *slice[MAX_SLICES];
    int slice_x_mod, slice_y_mod, slice_z_mod;
    IFLAG slice_draw_mode;

    void *clientdata;
} geovol;

struct lightdefs
{
    float position[4];		/* X, Y, Z, (1=local/0=inf) */
    float color[3];		/* R, G, B */
    float ambient[3];		/* R, G, B */
    float emission[3];		/* R, G, B */
    float shine;		/* 0. to 128. */
};

typedef struct
{
    int coord_sys;		/* latlon, equal area, etc */
    int view_proj;		/* perspective, ortho */
    int infocus;		/* fixed center of view - true or false */
    float from_to[2][4];
    int twist, fov, incl, look;	/* 10ths of degrees */
    float real_to[4], vert_exag;	/* a global Z exag */
    float scale;
    struct lightdefs lights[MAX_LIGHTS];
} geoview;

typedef struct
{				/* need to add elements here for off_screen drawing */
    float nearclip, farclip, aspect;
    short left, right, bottom, top;	/* Screen coordinates */
    int bgcol;
} geodisplay;

extern void (*Cxl_func) ();
extern void (*Swap_func) ();

/* Bring all the function prototypes */
#include "ogsf_proto.h"

#endif /* _GSTYPES_H */
