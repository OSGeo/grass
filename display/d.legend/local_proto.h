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
void draw_histogram(const char *, int, int, int, int, int, int, int, int, int);

/* get_stats.c */
void get_stats(const char *, struct stat_list *, int, int);
void run_stats(const char *, int, const char *, int);
