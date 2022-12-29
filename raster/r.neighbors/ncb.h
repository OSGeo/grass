#include <grass/raster.h>

struct ncb			/* neighborhood control block */
{
    DCELL **buf;		/* for reading raster map */
    int *value;			/* neighborhood values */
    int nsize;			/* size of the neighborhood */
    int dist;			/* nsize/2 */
    struct Categories cats;
    char **mask;
    DCELL **weights;
    const char *oldcell;
};

extern struct ncb ncb;
