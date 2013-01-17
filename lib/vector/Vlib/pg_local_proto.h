#ifndef __PG_LOCAL_PROTO_H__
#define __PG_LOCAL_PROTO_H__

#include <grass/vector.h>

#ifdef HAVE_POSTGRES
#include <libpq-fe.h>

#define CURSOR_PAGE 500

/*! Topological access */
#define TOPO_SCHEMA "topology"
#define TOPO_ID     "topology_id"
#define TOPO_TABLE  "grass"
#define TOPO_BBOX   "bbox"

#define SWAP32(x) \
        ((unsigned int)( \
            (((unsigned int)(x) & (unsigned int)0x000000ffUL) << 24) | \
            (((unsigned int)(x) & (unsigned int)0x0000ff00UL) <<  8) | \
            (((unsigned int)(x) & (unsigned int)0x00ff0000UL) >>  8) | \
            (((unsigned int)(x) & (unsigned int)0xff000000UL) >> 24) ))

#define SWAPDOUBLE(x) \
{                                                                 \
    unsigned char temp, *data = (unsigned char *) (x);            \
                                                                  \
    temp = data[0];                                               \
    data[0] = data[7];                                            \
    data[7] = temp;                                               \
    temp = data[1];                                               \
    data[1] = data[6];                                            \
    data[6] = temp;                                               \
    temp = data[2];                                               \
    data[2] = data[5];                                            \
    data[5] = temp;                                               \
    temp = data[3];                                               \
    data[3] = data[4];                                            \
    data[4] = temp;                                               \
}                                                                    

#define LSBWORD32(x)      (x)
#define MSBWORD32(x)      SWAP32(x)

/* used for building pseudo-topology (requires some extra information
 * about lines in cache) */
struct feat_parts
{
    int             a_parts; /* number of allocated items */
    int             n_parts; /* number of parts which forms given feature */
    SF_FeatureType *ftype;   /* simple feature type */
    int            *nlines;  /* number of lines used in cache */
    int            *idx;     /* index in cache where to start */
};

/* functions used in *_pg.c files */
int Vect__execute_pg(PGconn *, const char *);
int Vect__execute_get_value_pg(PGconn *, const char *);
SF_FeatureType Vect__cache_feature_pg(const char *, int, int,
                                      struct Format_info_cache *,
                                      struct feat_parts *);
int Vect__set_initial_query_pg(struct Format_info_pg *, int);
int Vect__load_plus_pg(struct Map_info *, int);

off_t V2__write_node_pg(struct Map_info *, const struct line_pnts *);
off_t V2__write_area_pg(struct Map_info *, const struct line_pnts *,
                        const struct line_cats *,
                        const struct line_pnts **, int);

int Vect__insert_face_pg(struct Map_info *, int);

#endif /* HAVE_POSTGRES */

#endif /* __PG_LOCAL_PROTO_H__ */
