
struct ncb			/* neighborhood control block */
{
    DCELL **buf;		/* for reading raster map */
    int *value;			/* neighborhood values */
    int nsize;			/* size of the neighborhood */
    int dist;			/* nsize/2 */
    struct Categories cats;
    char title[1024];
    FILE *out;
    char **mask;
    DCELL **weights;
    struct
    {
	char *name;
	char *mapset;
    }
    oldcell, newcell;
};

extern struct ncb ncb;
