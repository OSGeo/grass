struct stat_node
{
    long int cat;		/* cell-file category value */
    long int stat;		/* statistic: number of cells with that cat */
    struct stat_node *next;	/* pointer to next stat_node in list */
};

struct stat_list
{
    struct stat_node *ptr;	/* pointer to first stat_node in list */
    long int count,		/* number of stat_nodes in list 
				   (not counting null cells) */
      null_stat,		/* stats for null cell */
      maxstat,			/* max. statistic in list */
      minstat,			/* min. statistic in list */
      sumstat,			/* sum of all statistics in list */
      maxcat,			/* max. cell-file category value in list */
      mincat;			/* min. cell-file category value in list */
};

/* structures for determining tic-mark numbering scheme */
struct units
{
    char *name;			/* name of unit (text) */
    long int unit;		/* tic-mark interval */
    long int every;		/* tic_mark number interval */
};

extern struct Categories cats;
extern struct FPRange fp_range;

/* bar.c */
int bar(struct stat_list *, struct Colors *);
float rem(long int, long int);

/* draw_slice.c */
int draw_slice_unfilled(struct Colors *, int, double, double, double, double,
			double);
int draw_slice_filled(struct Colors *, DCELL, int, double, double, double,
		      double, double);
int draw_slice(struct Colors *, int, DCELL, DCELL, int, double, double,
	       double, double, double);

/* get_stats.c */
int get_stats(const char *, struct stat_list *);

/* pie.c */
int pie(struct stat_list *, struct Colors *);
