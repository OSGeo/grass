#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PHOTO_H__

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

/* Put all the state infomation into a struct */
struct rr_state
{
    char *inraster, *inrcover, *outraster, *mapset, *cmapset, *outvector;
    int use_nulls, docover, fd_old, fd_cold, fd_new;
    long nCells, nNulls, nRand, cnCells, cnNulls;
    struct RASTER_MAP_PTR nulls, cnulls, buf, cover, min, max, cmin, cmax;
    FILE *fsites;
    int z_geometry;
};


/* count.c */
void get_stats(struct rr_state *);

/* creat_rand.c */
long make_rand(void);
void init_rand(void);
long make_rand(void);
void init_rand(void);

/* random.c */
int execute_random(struct rr_state *);

/* support.c */
int make_support(struct rr_state *, int, double);

#endif /* __LOCAL_PROTO_H__ */

/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
