#include <grass/gis.h>
#include <grass/raster.h>

<<<<<<< HEAD
<<<<<<< HEAD
void c_sum(DCELL *result, DCELL *values, int n, const void *closure UNUSED)
=======
void c_sum(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void c_sum(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
{
    DCELL sum;
    int count;
    int i;

    sum = 0.0;
    count = 0;

    for (i = 0; i < n; i++) {
        if (Rast_is_d_null_value(&values[i]))
            continue;

        sum += values[i];
        count++;
    }

    if (count == 0)
        Rast_set_d_null_value(result, 1);
    else
        *result = sum;
}

<<<<<<< HEAD
<<<<<<< HEAD
void w_sum(DCELL *result, DCELL (*values)[2], int n, const void *closure UNUSED)
=======
void w_sum(DCELL *result, DCELL (*values)[2], int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void w_sum(DCELL *result, DCELL (*values)[2], int n, const void *closure)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
{
    DCELL sum;
    DCELL count;
    int i;

    sum = 0.0;
    count = 0.0;

    for (i = 0; i < n; i++) {
        if (Rast_is_d_null_value(&values[i][0]))
            continue;

        sum += values[i][0] * values[i][1];
        count += values[i][1];
    }

    if (count == 0)
        Rast_set_d_null_value(result, 1);
    else
        *result = sum;
}
