#ifndef GRASS_NVIZ_H
#define GRASS_NVIZ_H

#include <grass/config.h>

/*** Windows headers ***/
#if defined(OPENGL_WINDOWS)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
#  include <winnt.h>
#  include <GL/gl.h>
#  include <GL/glext.h>

/*** X Window System headers ***/
#elif defined(OPENGL_X11)
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/Xatom.h>	/* for XA_RGB_DEFAULT_MAP atom */
#  define GL_GLEXT_PROTOTYPES
#  include <GL/glx.h>

/*** Mac headers ***/
#elif defined(OPENGL_AQUA)
#  if defined(OPENGL_AGL)
#    define Cursor QDCursor
#    include <AGL/agl.h>
#    undef Cursor
#  else
#    include <OpenGL/CGLTypes.h>
#    include <OpenGL/CGLCurrent.h>
#    include <OpenGL/OpenGL.h>
#  endif
#  include <ApplicationServices/ApplicationServices.h>

#else /* make sure only one platform defined */
#  error Unsupported platform, or confused platform defines...
#endif

#include <grass/ogsf.h>

#define MAP_OBJ_UNDEFINED 0
#define MAP_OBJ_SURF 1
#define MAP_OBJ_VOL 2
#define MAP_OBJ_VECT 3
#define MAP_OBJ_SITE 4

#define DRAW_COARSE 0
#define DRAW_FINE 1
#define DRAW_BOTH 2

/* quick draw mode */
#define DRAW_QUICK_SURFACE 0x01
#define DRAW_QUICK_VLINES  0x02
#define DRAW_QUICK_VPOINTS 0x04
#define DRAW_QUICK_VOLUME  0x08

#define RANGE (5 * GS_UNIT_SIZE)
#define RANGE_OFFSET (2 * GS_UNIT_SIZE)
#define ZRANGE (3 * GS_UNIT_SIZE)
#define ZRANGE_OFFSET (1 * GS_UNIT_SIZE)

#define DEFAULT_SURF_COLOR 0x33BBFF

#define FORMAT_PPM 1
#define FORMAT_TIF 2

/* data structures */
typedef struct
{
    int id;
    float brt;
    float r, g, b;
    float ar, ag, ab;		/* ambient rgb */
    float x, y, z, w;		/* position */
} light_data;

struct fringe_data
{
    int           id;
    unsigned long color;
    float         elev;
    int           where[4];
};

struct arrow_data
{
    unsigned long color;
    float	  size;
    float	  where[3];
};

struct scalebar_data
{
    int           id;
    unsigned long color;
    float	  size;
    float	  where[3];
};

typedef struct
{
    /* ranges */
    float zrange, xyrange;

    /* cplanes */
    int num_cplanes;
    int cur_cplane, cp_on[MAX_CPLANES];
    float cp_trans[MAX_CPLANES][3];
    float cp_rot[MAX_CPLANES][3];

    /* light */
    light_data light[MAX_LIGHTS];
    
    /* fringe */
    int num_fringes;
    struct fringe_data **fringe;

    /* north arrow */
    int draw_arrow;
    struct arrow_data *arrow;
    
    /* scalebar */
    int num_scalebars;
    struct scalebar_data **scalebar;
    
    /* background color */
    int bgcolor;

} nv_data;

struct render_window
{
#if defined(OPENGL_X11)
    Display *displayId;		/* display connection */
    GLXContext contextId;	/* GLX rendering context */
    Pixmap pixmap;
    GLXPixmap windowId;
#elif defined(OPENGL_AQUA)
#if defined(OPENGL_AGL)
    AGLPixelFormat pixelFmtId;
    AGLContext contextId;
    AGLPbuffer windowId;
#else
    CGLContextObj contextId;
#endif
#elif defined(OPENGL_WINDOWS)
    HDC displayId;		/* display context */
    HGLRC contextId;		/* rendering context */
#endif
    int width, height;
};

#include <grass/defs/nviz.h>

#endif /* GRASS_NVIZ_H */
