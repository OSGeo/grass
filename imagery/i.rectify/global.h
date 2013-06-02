/* These next defines determine the size of the sub-window that will
 * be held in memory.  Larger values will require
 * more memory (but less i/o) If you increase these values, keep  in
 * mind that although you think the i/o will decrease, system paging
 * (which goes on behind the scenes) may actual increase the i/o.
 */
#include <grass/gis.h>
#include <grass/imagery.h>

#define L2BDIM 6
#define BDIM (1<<(L2BDIM))
#define L2BSIZE (2*(L2BDIM))
#define BSIZE (1<<(L2BSIZE))
#define HI(i) ((i)>>(L2BDIM))
#define LO(i) ((i)&((BDIM)-1))

typedef DCELL block[BDIM][BDIM];

struct cache
{
    int fd;
    int stride;
    int nblocks;
    block **grid;
    block *blocks;
    int *refs;
};

extern char *seg_mb;

extern RASTER_MAP_TYPE map_type;
extern int *ref_list;
extern struct Ref ref;

typedef void (*func) (struct cache *, void *, int, double *, double *, struct Cell_head *);

extern func interpolate;		/* interpolation routine        */

struct menu
{
    func method;		/* routine to interpolate new value      */
    char *name;			/* method name                           */
    char *text;			/* menu display - full description       */
};

/* georef coefficients */

extern double E12[10], N12[10];
extern double E21[10], N21[10];

extern double *E12_t, *N12_t;
extern double *E21_t, *N21_t;

extern struct Control_Points cp;

/* DELETED WITH CRS MODIFICATIONS
   extern double E12a, E12b, E12c, N12a, N12b, N12c;
   extern double E21a, E21b, E21c, N21a, N21b, N21c;
 */
extern struct Cell_head target_window;

/* cp.c */
int get_control_points(char *, int);

/* env.c */
int select_current_env(void);
int select_target_env(void);
int show_env(void);

/* exec.c */
int exec_rectify(int, char *, char *);

/* get_wind.c */
int get_target_window(int);
int georef_window(struct Cell_head *, struct Cell_head *, int, double);

/* rectify.c */
int rectify(char *, char *, char *, int, char *);

/* readcell.c */
extern struct cache *readcell(int, const char *);
extern block *get_block(struct cache *, int);

#define BKIDX(c,y,x) ((y) * (c)->stride + (x))
#define BKPTR(c,y,x) ((c)->grid[BKIDX((c),(y),(x))])
#define BLOCK(c,y,x) (BKPTR((c),(y),(x)) ? BKPTR((c),(y),(x)) : get_block((c),BKIDX((c),(y),(x))))
#define CPTR(c,y,x) (&(*BLOCK((c),HI((y)),HI((x))))[LO((y))][LO((x))])

/* report.c */
int report(long, int);

/* target.c */
int get_target(char *);

/* declare resampling methods */
/* bilinear.c */
extern void p_bilinear(struct cache *, void *, int, double *, double *,
		       struct Cell_head *);
/* cubic.c */
extern void p_cubic(struct cache *, void *, int, double *, double *,
		    struct Cell_head *);
/* nearest.c */
extern void p_nearest(struct cache *, void *, int, double *, double *,
		      struct Cell_head *);
/* bilinear_f.c */
extern void p_bilinear_f(struct cache *, void *, int, double *, double *,
		       struct Cell_head *);
/* cubic_f.c */
extern void p_cubic_f(struct cache *, void *, int, double *, double *,
		    struct Cell_head *);
/* lanczos.c */
extern void p_lanczos(struct cache *, void *, int, double *, double *,
		    struct Cell_head *);
extern void p_lanczos_f(struct cache *, void *, int, double *, double *,
		    struct Cell_head *);
