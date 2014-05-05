#include <grass/raster3d.h>

#define COMPARE_PRECISION 0.000000001

/* statistic table row struct */
typedef struct
{
    double min, max, vol, perc;
    int num, count;
} stat_row;

/* the statistic table struct */
typedef struct
{
    stat_row **table;
    stat_row *null;
    int sum_count, nsteps, equal;
    double sum_vol, sum_perc;
} stat_table;

/* the equal value struct */
typedef struct
{
    double val;			/* the equal value */
    int count;			/* the appearance count */
} equal_val;

/* an array of groups with equal values */
typedef struct
{
    equal_val **values;
    int count;
} equal_val_array;

/* prototypes */
equal_val_array *alloc_equal_val_array(int count);
void free_equal_val_array(equal_val_array *vals);
equal_val_array *add_equal_val_to_array(equal_val_array *array, double val);
int check_equal_value(equal_val_array * array, double val);
stat_table *create_stat_table(int nsteps, equal_val_array *values,
 			      double min, double max);
void free_stat_table(stat_table *stats);
void print_stat_table(stat_table *stats, int);
void update_stat_table(stat_table *stats, RASTER3D_Region *region);
void heapsort_eqvals(equal_val_array *data, int n);
void downheap_eqvals(equal_val_array *data, int n, int k);
void check_range_value(stat_table *stats, double value);
void tree_search_range(stat_table *stats, int left, int right,
		       double value);
