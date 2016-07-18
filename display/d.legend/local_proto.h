#include <grass/raster.h>

#define MAP_TYPE_RASTER2D 1
#define MAP_TYPE_RASTER3D 2
/* possibles for the future:
   #define MAP_TYPE_VECTOR 3
   #define MAP_TYPE_RULES 4
 */

struct stat_node
{
    long int cat;               /* cell-file category value */
    long int stat;              /* statistic: number of cells with that cat */
    struct stat_node *next;     /* pointer to next stat_node in list */
};

struct stat_list
{
    struct stat_node *ptr;      /* pointer to first stat_node in list */
    long int count,             /* number of stat_nodes in list 
                                   (not counting null cells) */
      null_stat,                /* stats for null cell */
      maxstat,                  /* max. statistic in list */
      minstat,                  /* min. statistic in list */
      sumstat,                  /* sum of all statistics in list */
      maxcat,                   /* max. cell-file category value in list */
      mincat;                   /* min. cell-file category value in list */
};


/* histogram.c */
double histogram(const char *, int, int, int, int, int, int, int, int,
                 int, struct FPRange, int);

/* get_stats.c */
void get_stats(const char *, struct stat_list *, int, int);
void run_stats(const char *, int, const char *, int);

/* draw.c */
void draw(const char *, int, int, int, int, int, int, int, int, int, int, int,
          int, struct Categories, struct Colors, double, double, double,
          double, int, int, double, double, double *, int, int, int, double,
          double, const char *, double *, double, int, int, struct Option *,
          struct Option *, struct Option *, struct Option *, struct Option *,
          struct Flag *, struct Flag *, int, int, int, char *);
