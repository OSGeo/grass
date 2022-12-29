#include <grass/raster.h>

#undef MIN
#undef MAX
#define MIN(a,b)      ((a) < (b) ? (a) : (b))
#define MAX(a,b)      ((a) > (b) ? (a) : (b))

struct input
{
    const char *name;
    const char *mapset;
    int maptype;
    struct Cell_head cellhd;
};

/* link.c */
void make_cell(const char *, int);
void make_link(const struct input *, int, const char *);
void write_fp_format(const char *, int);
void write_fp_quant(const char *);
void create_map(const struct input *, int, const char *,
		struct Cell_head *, int, DCELL, DCELL,
		int, struct R_stats *, const char *);
