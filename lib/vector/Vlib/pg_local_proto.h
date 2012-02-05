#ifndef __PG_LOCAL_PROTO_H__
#define __PG_LOCAL_PROTO_H__

#ifdef HAVE_POSTGRES
#include <libpq-fe.h>

/* functions used in *_pg.c files */
int execute(PGconn *, const char *);
int cache_feature(const char *, int,
		  struct Format_info_pg *);

#endif /* HAVE_POSTGRES */

#endif /* __PG_LOCAL_PROTO_H__ */
