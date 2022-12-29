#ifndef _V_EDIT_GLOBAL
# define _V_EDIT_GLOBAL

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <grass/gis.h>
# include <grass/vector.h>
# include <grass/glocale.h>
# include <grass/vedit.h>

# define SEP "----------------------------------"

#define THRESH_COORDS 0
#define THRESH_SNAP   1
#define THRESH_QUERY  2

enum mode
{
    /* geometry of features is not changed */
    MODE_CREATE,		/* create new vector map */
    MODE_ADD,			/* add new vector features */
    MODE_DEL,			/* delete new vector features */
    MODE_COPY,			/* copy vector features */
    MODE_MOVE,			/* move vector features */
    MODE_FLIP,			/* flip direction of vector lines */
    MODE_CATADD,		/* add layer category */
    MODE_CATDEL,		/* delete layer category */
    /* geometry of features changed */
    MODE_MERGE,			/* merge vector lines */
    MODE_BREAK,			/* break (split) vector lines */
    MODE_CONNECT,		/* connect *two* lines */
    MODE_EXTEND,		/* extend both nodes */
    MODE_EXTEND_START,		/* extend start node */
    MODE_EXTEND_END,		/* extend end node */
    MODE_SNAP,			/* snap vector lines */
    /* geometry of feature changed */
    MODE_VERTEX_ADD,		/* add vertex */
    MODE_VERTEX_DELETE,		/* delete vertex(ces) */
    MODE_VERTEX_MOVE,		/* move vertex(ces) */
    /* only print selected features */
    MODE_SELECT,
    /* */
    MODE_NONE,
    /* z bulk-labeling */
    MODE_ZBULK,
    /* change feature type (point<->centroid, line<->boundary) */
    MODE_CHTYPE,
    MODE_AREA_DEL,              /* delete area */
};

struct GParams
{
    struct Option *map, *in, *maxdist, *tool,
	*coord, *cat, *move, *bbox, *fld,
	*poly, *type, *id, *where, *bmaps, *snap, *query, *zbulk;
    struct Flag *header, *topo, *close, *reverse, *move_first, *extend_parallel;
};

# include "proto.h"

#endif
