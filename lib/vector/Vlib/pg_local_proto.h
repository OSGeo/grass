#ifndef __PG_LOCAL_PROTO_H__
#define __PG_LOCAL_PROTO_H__

#ifdef HAVE_POSTGRES
#include <libpq-fe.h>

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
int execute(PGconn *, const char *);
SF_FeatureType cache_feature(const char *, int,
			     struct Format_info_cache *,
			     struct feat_parts *);

#endif /* HAVE_POSTGRES */

#endif /* __PG_LOCAL_PROTO_H__ */
