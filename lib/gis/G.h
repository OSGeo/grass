#include <grass/config.h>
#include <grass/gis.h>

#ifdef GRASS_CMAKE_BUILD
#include <export/grass_gis_export.h>
#else
#define GRASS_GIS_EXPORT
#endif

struct G__ /*  Structure of library globals */
{
    struct Cell_head window; /* Contains the current window          */
    int window_set;          /* Flag: window set?                    */
    int little_endian;       /* Flag denoting little-endian architecture */
    int compression_level;   /* zlib compression level               */
};

GRASS_GIS_EXPORT extern struct G__ G__; /* allocated in gisinit */
