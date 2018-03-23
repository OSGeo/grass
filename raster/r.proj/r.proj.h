/* @(#)r.proj.h v1.2 - 27 Jun 1995      -emes- */

#ifndef R_PROJ_H
#define R_PROJ_H

#include <grass/gprojects.h>

#define L2BDIM 6
#define BDIM (1<<(L2BDIM))
#define L2BSIZE (2*(L2BDIM))
#define BSIZE (1<<(L2BSIZE))
#define HI(i) ((i)>>(L2BDIM))
#define LO(i) ((i)&((BDIM)-1))

typedef FCELL block[BDIM][BDIM];

struct cache
{
    int fd;
    char *fname;
    int stride;
    int nblocks;
    block **grid;
    block *blocks;
    int *refs;
};

typedef void (*func) (struct cache *, void *, int, double, double,
		      struct Cell_head *);

struct menu
{
    func method;		/* routine to interpolate new value      */
    char *name;			/* method name                           */
    char *text;			/* menu display - full description       */
};

extern void bordwalk(const struct Cell_head *, struct Cell_head *,
		     const struct pj_info *, const struct pj_info *,
		     const struct pj_info *, int);
extern void bordwalk_edge(const struct Cell_head *, struct Cell_head *,
		          const struct pj_info *, const struct pj_info *,
			  const struct pj_info *, int);
extern struct cache *readcell(int, const char *);
extern block *get_block(struct cache *, int);
extern void release_cache(struct cache *);

/* declare resampling methods */
/* bilinear.c */
extern void p_bilinear(struct cache *, void *, int, double, double,
		       struct Cell_head *);
/* cubic.c */
extern void p_cubic(struct cache *, void *, int, double, double,
		    struct Cell_head *);
/* nearest.c */
extern void p_nearest(struct cache *, void *, int, double, double,
		      struct Cell_head *);
/* bilinear_f.c */
extern void p_bilinear_f(struct cache *, void *, int, double, double,
		       struct Cell_head *);
/* cubic_f.c */
extern void p_cubic_f(struct cache *, void *, int, double, double,
		    struct Cell_head *);
/* lanczos.c */
extern void p_lanczos(struct cache *, void *, int, double, double,
		    struct Cell_head *);
extern void p_lanczos_f(struct cache *, void *, int, double, double,
		    struct Cell_head *);

#if 1

#define BKIDX(c,y,x) ((y) * (c)->stride + (x))
#define BKPTR(c,y,x) ((c)->grid[BKIDX((c),(y),(x))])
#define BLOCK(c,y,x) (BKPTR((c),(y),(x)) ? BKPTR((c),(y),(x)) : get_block((c),BKIDX((c),(y),(x))))
#define CVAL(c,y,x) ((*BLOCK((c),HI((y)),HI((x))))[LO((y))][LO((x))])

#else

static inline int BKIDX(const struct cache *c, int y, int x)
{
    return y * c->stride + x;
}

static inline block *BKPTR(const struct cache *c, int y, int x)
{
    return c->grid[BKIDX(c, y, x)];
}

static inline block *BLOCK(struct cache *c, int y, int x)
{
    return BKPTR(c, y, x) ? BKPTR(c, y, x) : get_block(c, BKIDX(c, y, x));
}

static inline FCELL *CPTR(struct cache *c, int y, int x)
{
    return &(*BLOCK(c, HI(y), HI(x)))[LO(y)][LO(x)];
}

#endif

#endif
