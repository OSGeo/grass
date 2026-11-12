/* @(#)r.proj.h v1.2 - 27 Jun 1995      -emes- */

#ifndef R_PROJ_H
#define R_PROJ_H

#include <grass/gprojects.h>

#define L2BDIM  6
#define BDIM    (1 << (L2BDIM))
#define L2BSIZE (2 * (L2BDIM))
#define BSIZE   (1 << (L2BSIZE))
#define HI(i)   ((i) >> (L2BDIM))
#define LO(i)   ((i) & ((BDIM) - 1))

typedef FCELL block[BDIM][BDIM];

struct cache {
    int fd;
    char *fname;
    int stride;
    int nblocks;
    block **grid;
    block *blocks;
    int *refs;
};

typedef void (*func)(struct cache *, void *, int, double, double,
                     struct Cell_head *);

/* One input strip resident in memory, addressed by full-map input row. Rows
 * outside imin to imax are not loaded. A kernel that needs one records it
 * through the need arguments so the band reloads and recomputes. */
struct strip {
    void *data;     /* input rows imin to imax at full input width */
    int imin, imax; /* loaded input row range */
    int cols;       /* input columns, the row stride */
};

/* Strip interpolation kernels (interp_strip.c) read from an in-memory strip.
 * The last two arguments carry the widest input rows a kernel wanted below imin
 * and above imax, left unchanged when the strip covered every sample. */
typedef void (*strip_func)(const struct strip *, void *, int, double, double,
                           struct Cell_head *, int *, int *);

struct menu {
    func method; /* routine to interpolate new value      */
    char *name;  /* method name                           */
    char *text;  /* menu display - full description       */
};

enum OutputFormat { PLAIN, SHELL, JSON };

/* Footprint of input row spans for the output map, built in footprint.c. The
 * struct is private to footprint.c. */
struct footprint;
extern struct footprint *
fp_create(const struct Cell_head *ohd, const struct Cell_head *ihd,
          const struct pj_info *oproj, const struct pj_info *iproj,
          const struct pj_info *tproj, const double *y_center);
extern void fp_span(const struct footprint *g, int first_row, int end_row,
                    int first_col, int end_col, int *imin, int *imax);
extern int fp_num_blocks(const struct footprint *g);
extern int fp_block_start(const struct footprint *g, int b);
extern int fp_band_geometry(const struct footprint *g, int first_row,
                            size_t cap_bytes, int out_mult, int cell_size,
                            int in_cols, int *tile_blocks_out,
                            int *worst_block_rows);
extern void fp_free(struct footprint *g);

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

/* interp_strip.c - strip versions of the resampling methods */
extern void strip_bilinear(const struct strip *, void *, int, double, double,
                           struct Cell_head *, int *, int *);
extern void strip_cubic(const struct strip *, void *, int, double, double,
                        struct Cell_head *, int *, int *);
extern void strip_lanczos(const struct strip *, void *, int, double, double,
                          struct Cell_head *, int *, int *);
extern void strip_bilinear_f(const struct strip *, void *, int, double, double,
                             struct Cell_head *, int *, int *);
extern void strip_cubic_f(const struct strip *, void *, int, double, double,
                          struct Cell_head *, int *, int *);
extern void strip_lanczos_f(const struct strip *, void *, int, double, double,
                            struct Cell_head *, int *, int *);

#define BKIDX(c, y, x) ((y) * (c)->stride + (x))
#define BKPTR(c, y, x) ((c)->grid[BKIDX((c), (y), (x))])
#define BLOCK(c, y, x)                           \
    (BKPTR((c), (y), (x)) ? BKPTR((c), (y), (x)) \
                          : get_block((c), BKIDX((c), (y), (x))))
#define CVAL(c, y, x) ((*BLOCK((c), HI((y)), HI((x))))[LO((y))][LO((x))])

#endif
