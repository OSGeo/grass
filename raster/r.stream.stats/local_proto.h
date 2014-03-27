#include "io.h"
#include "local_vars.h"

/* stats prepare */
int fifo_insert(POINT point);
POINT fifo_return_del(void);
/* ram version */
int ram_init_streams(CELL **streams, CELL **dirs, FCELL **elevation);
int ram_calculate_streams(CELL **streams, CELL **dirs, FCELL **elevation);
double ram_calculate_basins_area(CELL **dirs, int r, int c);
int ram_calculate_basins(CELL **dirs);
/* seg version */
int seg_init_streams(SEGMENT *streams, SEGMENT *dirs, SEGMENT *elevation);
int seg_calculate_streams(SEGMENT *streams, SEGMENT *dirs, SEGMENT *elevation);
double seg_calculate_basins_area(SEGMENT *dirs, int r, int c);
int seg_calculate_basins(SEGMENT *dirs);

/* stats calculate */
double stats_linear_reg(int max_order, double* statistic);
int stats(int order_max);

/* stats print */
int print_stats(int order_max);
int print_stats_total(void);
int print_stats_orders(int order_max);
