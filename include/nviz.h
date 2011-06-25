#ifndef GRASS_NVIZ_H
#define GRASS_NVIZ_H

#include <grass/config.h>

/*** Windows headers ***/
#if defined(OPENGL_WINDOWS)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
#  include <winnt.h>

/*** X Window System headers ***/
#elif defined(OPENGL_X11)
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/Xatom.h>	/* for XA_RGB_DEFAULT_MAP atom */
#  if defined(__vms)
#    include <X11/StdCmap.h>	/* for XmuLookupStandardColormap */
#  else
#    include <X11/Xmu/StdCmap.h>	/* for XmuLookupStandardColormap */
#  endif
#  include <GL/glx.h>

/*** Mac headers ***/
#elif defined(OPENGL_AQUA)
#  define Cursor QDCursor
#  include <AGL/agl.h>
#  undef Cursor
#  include <ApplicationServices/ApplicationServices.h>

#else /* make sure only one platform defined */
#  error Unsupported platform, or confused platform defines...
#endif

#include <grass/gsurf.h>
#include <grass/gstypes.h>

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
    
    /* background color */
    int bgcolor;

} nv_data;

struct render_window
{
#if defined(OPENGL_X11)
    Display *displayId;		/* display connection */
    GLXContext contextId;	/* GLX rendering context */
    GLXPixmap windowId;
    Pixmap pixmap;
#elif defined(OPENGL_AQUA)
    AGLPixelFormat pixelFmtId;
    AGLContext contextId;
    AGLPbuffer windowId;
#elif defined(OPENGL_WINDOWS)
    HDC displayId;		/* display context */
    HGLRC contextId;		/* rendering context */
    HBITMAP bitmapId;
#endif
};

/* change_view.c */
int Nviz_resize_window(int, int);
int Nviz_update_ranges(nv_data *);
int Nviz_set_viewpoint_position(double, double);
int Nviz_set_viewpoint_height(double);
int Nviz_set_viewpoint_persp(int);
int Nviz_set_viewpoint_twist(int);
int Nviz_change_exag(nv_data *, double);
int Nviz_look_here(double, double);

/* cplanes_obj.c */
int Nviz_new_cplane(nv_data *, int);
int Nviz_on_cplane(nv_data *, int);
int Nviz_off_cplane(nv_data *, int);
int Nviz_draw_cplane(nv_data *, int, int);
int Nviz_num_cplanes(nv_data *);
int Nviz_get_current_cplane(nv_data *);
int Nviz_set_cplane_rotation(nv_data *, int, float, float, float);
int Nviz_get_cplane_rotation(nv_data *, int, float *, float *, float *);
int Nviz_set_cplane_translation(nv_data *, int, float, float, float);
int Nviz_get_cplane_translation(nv_data *, int, float *, float *, float *);
int Nviz_set_fence_color(nv_data *, int);


/* draw.c */
int Nviz_draw_all_surf(nv_data *);
int Nviz_draw_all_vect();
int Nviz_draw_all_site();
int Nviz_draw_all_vol();
int Nviz_draw_all(nv_data *);
int Nviz_draw_quick(nv_data *, int);

/* exag.c */
int Nviz_get_exag_height(double *, double *, double *);
double Nviz_get_exag();

/* lights.c */
int Nviz_set_light_position(nv_data *, int, double, double, double, double);
int Nviz_set_light_bright(nv_data *, int, double);
int Nviz_set_light_color(nv_data *, int, int, int, int);
int Nviz_set_light_ambient(nv_data *, int, double);
int Nviz_init_light(nv_data *, int);
int Nviz_new_light(nv_data *);

/* map_obj.c */
int Nviz_new_map_obj(int, const char *, double, nv_data *);
int Nviz_set_attr(int, int, int, int, const char *, double, nv_data *);
void Nviz_set_surface_attr_default();
int Nviz_set_vpoint_attr_default();
int Nviz_set_volume_attr_default();
int Nviz_unset_attr(int, int, int);

/* nviz.c */
void Nviz_init_data(nv_data *);
void Nviz_destroy_data(nv_data *);
void Nviz_set_bgcolor(nv_data *, int);
int Nviz_get_bgcolor(nv_data *);
int Nviz_color_from_str(const char *);
struct fringe_data *Nviz_new_fringe(nv_data *, int, unsigned long,
				    double, int, int, int, int);
struct fringe_data *Nviz_set_fringe(nv_data *, int, unsigned long,
				    double, int, int, int, int);

/* position.c */
void Nviz_init_view(nv_data *);
int Nviz_set_focus_state(int);
int Nviz_set_focus_map(int, int);
int Nviz_has_focus(nv_data *);
int Nviz_set_focus(nv_data *, float, float, float);
int Nviz_get_focus(nv_data *, float *, float *, float *);
float Nviz_get_xyrange(nv_data *);
int Nviz_get_zrange(nv_data *, float *, float *);

/* render.c */
struct render_window *Nviz_new_render_window();
void Nviz_init_render_window(struct render_window *);
void Nviz_destroy_render_window(struct render_window *);
int Nviz_create_render_window(struct render_window *, void *, int, int);
int Nviz_make_current_render_window(const struct render_window *);

#endif /* GRASS_NVIZ_H */
