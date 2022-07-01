/*!
  \file include/ogsf.h

  \brief OGSF header file (structures)

  This program is free software under the GNU General
  Public License (>=v2). Read the file COPYING that
  comes with GRASS for details.
  
  \author Original author Bill Brown, USACERL (January 1993)
  \author Thematic mapping enabled by Martin Landa <landa.martin gmail.com) (06/2011)

  (C) 2011 by the GRASS Development Team
*/

#ifndef GRASS_OGSF_H
#define GRASS_OGSF_H

#include <grass/config.h>
#include <grass/bitmap.h>
#if defined(OPENGL_X11) || defined(OPENGL_WINDOWS)
#include <GL/gl.h>
#endif
#ifdef OPENGL_AQUA
#include <OpenGL/gl.h>
#endif

#include <grass/gis.h>

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
#define VOL_FTYPE_RASTER3D        0

/* types of volume values */
#define VOL_DTYPE_FLOAT     0
#define VOL_DTYPE_DOUBLE    1

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
    size_t numbytes;
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
    int active;
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
    int use_z;
    int n_surfs;
    char *filename;
    float x_trans, y_trans, z_trans;
    /* also maybe center & rotate? */
    geoline *lines;
    geoline *fastlines;
    int (*bgn_read) (), (*end_read) (), (*nxt_line) ();
    struct g_vect *next;
    void *clientdata;
  
    gvstyle_thematic *tstyle;  /* thematic mapping */
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
    int draw_wire;

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

struct georot
{
    int do_rot;			/* do rotation */
    double rot_angle;		/* rotation angle */
    double rot_axes[3];		/* rotation axis */
    GLdouble rotMatrix[16];	/* rotation matrix */
};

typedef struct
{
    int coord_sys;		/* latlon, equal area, etc */
    int view_proj;		/* perspective, ortho */
    int infocus;		/* fixed center of view - true or false */
    float from_to[2][4];
    struct georot rotate;
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

/* Key frames */
/* these have to be 1 << KF_id_index */

#define KF_FROMX_MASK	0x00000001
#define KF_FROMY_MASK	0x00000002
#define KF_FROMZ_MASK	0x00000004
#define KF_FROM_MASK	0x00000007

#define KF_DIRX_MASK	0x00000008
#define KF_DIRY_MASK	0x00000010
#define KF_DIRZ_MASK	0x00000020
#define KF_DIR_MASK	0x00000038

#define KF_FOV_MASK	0x00000040
#define KF_TWIST_MASK	0x00000080

#define KF_ALL_MASK	0x000000FF

#define KF_NUMFIELDS 8

#define KF_LINEAR 111
#define KF_SPLINE 222
#define KF_LEGAL_MODE(m) (m == KF_LINEAR || m == KF_SPLINE)

#define KF_FROMX 0
#define KF_FROMY 1
#define KF_FROMZ 2
#define KF_DIRX 3
#define KF_DIRY 4
#define KF_DIRZ 5
#define KF_FOV 6
#define KF_TWIST 7

#define FM_VECT 0x00000001
#define FM_SITE 0x00000002
#define FM_PATH 0x00000004
#define FM_VOL  0x00000008
#define FM_LABEL 0x00000010

typedef struct view_node
{
    float fields[KF_NUMFIELDS];
} Viewnode;

typedef struct key_node
{
    float pos, fields[KF_NUMFIELDS];
    int look_ahead;
    unsigned long fieldmask;
    struct key_node *next, *prior;
} Keylist;

/* Bring all the function prototypes */
#include <grass/defs/ogsf.h>

#endif /* GRASS_OGSF_H */
