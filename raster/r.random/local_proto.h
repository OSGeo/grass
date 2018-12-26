#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PHOTO_H__

#include <grass/raster.h>

/* raster_ptr.c: From Huidae Cho ... */
union RASTER_PTR
{
    void *v;
    CELL *c;
    FCELL *f;
    DCELL *d;
};

struct RASTER_MAP_PTR
{
    RASTER_MAP_TYPE type;
    union RASTER_PTR data;
};

int is_null_value(struct RASTER_MAP_PTR buf, int col);

/* End from Huidae Cho */

#ifdef HAVE_LONG_LONG_INT
typedef unsigned long long gcell_count;
#else
typedef unsigned long gcell_count;
#endif

/* Put all the state information into a struct */
struct rr_state
{
    char *inraster, *inrcover, *outraster, *outvector;
    int use_nulls, docover, fd_old, fd_cold, fd_new;
    gcell_count nCells, nNulls, nRand, cnCells, cnNulls;
    struct RASTER_MAP_PTR nulls, cnulls, buf, cover, min, max, cmin, cmax;
    FILE *fsites;
    int z_geometry;
    int notopol;
};


/* count.c */
void get_stats(struct rr_state *);

/* random.c */
int execute_random(struct rr_state *);

/* support.c */
int make_support(struct rr_state *, int, double);

#endif /* __LOCAL_PROTO_H__ */

/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
