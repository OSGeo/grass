#ifndef GRASS_STATSDEFS_H
#define GRASS_STATSDEFS_H

typedef void stat_func(DCELL *, DCELL *, int, const void *);
typedef void stat_func_w(DCELL *, DCELL(*)[2], int, const void *);

extern stat_func c_ave;
extern stat_func c_count;
extern stat_func c_divr;
extern stat_func c_intr;
extern stat_func c_max;
extern stat_func c_maxx;
extern stat_func c_median;
extern stat_func c_min;
extern stat_func c_minx;
extern stat_func c_mode;
extern stat_func c_stddev;
extern stat_func c_sum;
extern stat_func c_thresh;
extern stat_func c_var;
extern stat_func c_range;
extern stat_func c_reg_m;
extern stat_func c_reg_c;
extern stat_func c_reg_r2;
extern stat_func c_reg_t;
extern stat_func c_quart1;
extern stat_func c_quart3;
extern stat_func c_perc90;
extern stat_func c_quant;
extern stat_func c_skew;
extern stat_func c_kurt;

extern stat_func_w w_ave;
extern stat_func_w w_count;
extern stat_func_w w_median;
extern stat_func_w w_min;
extern stat_func_w w_max;
extern stat_func_w w_mode;
extern stat_func_w w_quart1;
extern stat_func_w w_quart3;
extern stat_func_w w_perc90;
extern stat_func_w w_quant;
extern stat_func_w w_reg_m;
extern stat_func_w w_reg_c;
extern stat_func_w w_reg_r2;
extern stat_func_w w_reg_t;
extern stat_func_w w_stddev;
extern stat_func_w w_sum;
extern stat_func_w w_var;
extern stat_func_w w_skew;
extern stat_func_w w_kurt;

extern int sort_cell(DCELL *, int);
extern int sort_cell_w(DCELL(*)[2], int);

#endif
