#include <grass/gis.h>
#include <grass/raster.h>

#define SORT_DEFAULT 0
#define SORT_ASC     1
#define SORT_DESC    2

extern char *no_data_str;
extern int nfiles;
extern int nrows;
extern int ncols, no_nulls, no_nulls_all;
extern int nsteps, cat_ranges, raw_output, as_int, averaged;
extern int *is_fp;
extern DCELL *DMAX, *DMIN;

extern CELL NULL_CELL;

extern char *fs;
extern struct Categories *labels;

/* cell_stats.c */
int cell_stats(int[], int, int, int, int, int, char *);

/* raw_stats.c */
int raw_stats(int[], int, int, int);

/* stats.c */
int initialize_cell_stats(int);
int allocate_values(void);
struct Node *NewNode(double);
void fix_max_fp_val(CELL *, int);
void reset_null_vals(CELL *, int);
int update_cell_stats(CELL **, int, double);
int sort_cell_stats(int);
int print_node_count(void);
int print_cell_stats(char *, int, int, int, int, char *);
