#define DISTRIB 0
#define AVERAGE 1
#define MODE    2
#define MEDIAN  3
#define ADEV    4		/* Average deviation     */
#define SDEV    5		/* Standard deviation    */
#define VARIANC 6		/* Variance              */
#define SKEWNES 7		/* Skewnes               */
#define KURTOSI 8		/* Kurtosis              */
#define MIN	9		/* Minimum               */
#define MAX	10		/* Maximum               */
#define SUM	11		/* Sum                   */
#define DIV	12		/* Diversity             */

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
    int val;			/* number of function */
    const char *text;		/* menu display - full description */
};

extern struct menu menu[];

/* o_adev.c */
int o_adev(const char *, const char *, const char *, int, struct Categories *);

/* o_average.c */
int o_average(const char *, const char *, const char *, int, struct Categories *);

/* o_distrib.c */
int o_distrib(const char *, const char *, const char *, int);

/* o_kurt.c */
int o_kurt(const char *, const char *, const char *, int, struct Categories *);

/* o_max.c */
int o_max(const char *, const char *, const char *, int, struct Categories *);

/* o_median.c */
int o_median(const char *, const char *, const char *, int, struct Categories *);

/* o_min.c */
int o_min(const char *, const char *, const char *, int, struct Categories *);

/* o_mode.c */
int o_mode(const char *, const char *, const char *, int, struct Categories *);

/* o_sdev.c */
int o_sdev(const char *, const char *, const char *, int, struct Categories *);

/* o_skew.c */
int o_skew(const char *, const char *, const char *, int, struct Categories *);

/* o_sum.c */
int o_sum(const char *, const char *, const char *, int, struct Categories *);

/* o_var.c */
int o_var(const char *, const char *, const char *, int, struct Categories *);

/* o_divr.c */
int o_divr(const char *, const char *, const char *, int, struct Categories *);

/* read_stats.c */
int read_stats(FILE *, long *, long *, double *);

/* write_rec.c */
int write_reclass(FILE *, long, long, char *, int);
