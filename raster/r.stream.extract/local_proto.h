
#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/raster.h>
#include "flag.h"
#include "seg.h"

#define GW_LARGE_INT off_t

#define INDEX(r, c) ((r) * ncols + (c))
#define MAXDEPTH 1000     /* maximum supported tree depth of stream network */

#define POINT       struct a_point
POINT {
    int r, c;
};

#define HEAP_PNT    struct heap_point
HEAP_PNT {
   GW_LARGE_INT added;
   CELL ele;
   POINT pnt;
};

#define WAT_ALT    struct wat_altitude
WAT_ALT {
   CELL ele;
   DCELL wat;
};

#define ASP_FLAG    struct aspect_flag
ASP_FLAG {
   char asp;
   char flag;
};

struct snode
{
    int r, c;
    int id;
    int n_trib;           /* number of tributaries */
    int n_trib_total;     /* number of all upstream stream segments */
    int n_alloc;          /* n allocated tributaries */
    int *trib;
};

/* extern variables */

extern int nrows, ncols;
extern GW_LARGE_INT n_search_points, n_points, nxt_avail_pt;
extern GW_LARGE_INT heap_size;
extern GW_LARGE_INT n_stream_nodes, n_alloc_nodes;
extern POINT *outlets;
extern struct snode *stream_node;
extern GW_LARGE_INT n_outlets, n_alloc_outlets;
extern char drain[3][3];
extern char sides;
extern int c_fac;
extern int ele_scale;
extern int have_depressions;

extern SSEG search_heap;
extern SSEG astar_pts;
extern SSEG watalt, aspflag;
/* extern BSEG bitflags, asp; */
extern CSEG stream;

/* load.c */
int load_maps(int, int);

/* init_search.c */
int init_search(int);

/* do_astar.c */
int do_astar(void);
GW_LARGE_INT heap_add(int, int, CELL);

/* streams.c */
int do_accum(double);
int extract_streams(double, double, int);

/* thin.c */
int thin_streams(void);

/* basins.c */
int basin_borders(void);

/* del_streams.c */
int del_streams(int);

/* close.c */
int close_maps(char *, char *, char *);

#endif /* __LOCAL_PROTO_H__ */
