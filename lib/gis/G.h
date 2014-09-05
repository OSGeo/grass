#include <grass/config.h>
#include <grass/gis.h>

struct G__			/*  Structure of library globals */
{
    struct Cell_head window;	/* Contains the current window          */
    int window_set;		/* Flag: window set?                    */
    int little_endian;          /* Flag denoting little-endian architecture */
};

extern struct G__ G__;		/* allocated in gisinit */
