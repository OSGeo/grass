#include <grass/gis.h>

#ifndef GLOBAL
# define GLOBAL extern
# define INIT(x)
#else
# define INIT(x) =(x)
#endif

GLOBAL char * no_data_str;
GLOBAL int nfiles;
GLOBAL int nrows;
GLOBAL int ncols, no_nulls INIT(0), no_nulls_all INIT(0);
GLOBAL int nsteps, cat_ranges, raw_output, as_int, averaged;
GLOBAL int *is_fp INIT(NULL);
GLOBAL DCELL *DMAX INIT(NULL), *DMIN INIT(NULL);

GLOBAL CELL NULL_CELL;
GLOBAL int (*get_row)();

GLOBAL char fs[2];
GLOBAL struct Categories *labels INIT(NULL) ;

/* cell_stats.c */
int cell_stats(int [], int, int, int, int, char *);
/* raw_stats.c */
int raw_stats(int [], int, int, int);
/* stats.c */
int initialize_cell_stats(int);
int allocate_values(void);
struct Node *NewNode(double);
int reset_null_vals(CELL *, int);
int update_cell_stats(CELL **, int, double);
int node_compare(const void *, const void *);
int sort_cell_stats(void);
int print_node_count(void);
int print_cell_stats(char *, int, int, int, int, char *);
