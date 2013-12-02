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
#define TOPO_TABLE_NUM  4
#define TOPO_TABLE_NODE "node_grass"
#define TOPO_TABLE_LINE "line_grass"
#define TOPO_TABLE_AREA "area_grass"
#define TOPO_TABLE_ISLE "isle_grass"

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

/* area_pg.c */
int Vect__get_area_points_pg(const struct Map_info *, const plus_t *, int, struct line_pnts *);

/* build_pg.c */
int Vect__clean_grass_db_topo(struct Format_info_pg *);

/* read_pg.c */
SF_FeatureType Vect__cache_feature_pg(const char *, int, int,
                                      struct Format_info_cache *,
                                      struct feat_parts *);
int Vect__open_cursor_next_line_pg(struct Format_info_pg *, int);
int Vect__open_cursor_line_pg(struct Format_info_pg *, int, int);
int Vect__close_cursor_pg(struct Format_info_pg *);
int Vect__select_line_pg(struct Format_info_pg *, int, int);
int Vect__execute_pg(PGconn *, const char *);
int Vect__execute_get_value_pg(PGconn *, const char *);
void Vect__reallocate_cache(struct Format_info_cache *, int, int);

/* write_pg.c */
off_t V2__write_node_pg(struct Map_info *, const struct line_pnts *);
off_t V2__write_area_pg(struct Map_info *, const struct line_pnts **, int,
                        const struct line_cats *);
int Vect__insert_face_pg(struct Map_info *, int);

/* open_pg.c */
int Vect__load_plus_head(struct Map_info *);
int Vect__load_plus_pg(struct Map_info *, int);
int Vect__open_new_pg(struct Map_info *, int);
int Vect__open_topo_pg(struct Map_info *, int, int);
int Vect__load_map_nodes_pg(struct Map_info *, int);
int Vect__load_map_lines_pg(struct Map_info *);

#endif /* HAVE_POSTGRES */

#endif /* __PG_LOCAL_PROTO_H__ */
