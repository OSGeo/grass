#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>

struct stats
{
    int nalloc;
    int n;
    long *cat;
    double *area;
};

struct menu
{
    const char *name;		/* method name */
    int (*func)(const char *, const char *, const char *, int, struct Categories *);
    const char *text;		/* menu display - full description */
};

extern struct menu menu[];

int o_adev(const char *, const char *, const char *, int, struct Categories *);
int o_average(const char *, const char *, const char *, int, struct Categories *);
int o_divr(const char *, const char *, const char *, int, struct Categories *);
int o_kurt(const char *, const char *, const char *, int, struct Categories *);
int o_max(const char *, const char *, const char *, int, struct Categories *);
int o_median(const char *, const char *, const char *, int, struct Categories *);
int o_min(const char *, const char *, const char *, int, struct Categories *);
int o_mode(const char *, const char *, const char *, int, struct Categories *);
int o_sdev(const char *, const char *, const char *, int, struct Categories *);
int o_skew(const char *, const char *, const char *, int, struct Categories *);
int o_sum(const char *, const char *, const char *, int, struct Categories *);
int o_var(const char *, const char *, const char *, int, struct Categories *);

/* run_cmd.c */

FILE *run_stats(struct Popen *, const char *, const char *, const char *);
FILE *run_reclass(struct Popen *, const char *, const char *);

/* read_stats.c */
int read_stats(FILE *, long *, long *, double *);

/* write_rec.c */
int write_reclass(FILE *, long, long, char *, int);
