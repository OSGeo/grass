
#ifndef GRASS_CALC_H
#define GRASS_CALC_H

#include <grass/gis.h>
#include <grass/raster.h>

typedef int func_t(int argc, const int *argt, void **args);
typedef int args_t(int argc, int *argt);

enum {
    E_ARG_LO	= 1,
    E_ARG_HI	= 2,
    E_ARG_TYPE	= 3,
    E_RES_TYPE	= 4,
    E_INV_TYPE	= 5,
    E_ARG_NUM	= 6,
    E_WTF	= 99
};

typedef struct func_desc
{
    const char *name;
    args_t *check_args;
    func_t *func;
} func_desc;

#define IS_NULL_C(x) (Rast_is_c_null_value((x)))
#define IS_NULL_F(x) (Rast_is_f_null_value((x)))
#define IS_NULL_D(x) (Rast_is_d_null_value((x)))

#define SET_NULL_C(x) (Rast_set_c_null_value((x),1))
#define SET_NULL_F(x) (Rast_set_f_null_value((x),1))
#define SET_NULL_D(x) (Rast_set_d_null_value((x),1))

extern volatile int floating_point_exception;
extern volatile int floating_point_exception_occurred;

extern int columns;

extern func_desc calc_func_descs[];

#include <grass/defs/calc.h>

#endif
