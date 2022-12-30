#include <grass/gis.h>
#include <grass/raster.h>

<<<<<<< HEAD
void c_count(DCELL *result, DCELL *values, int n, const void *closure UNUSED)
=======
void c_count(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    int count;
    int i;

    count = 0;

    for (i = 0; i < n; i++)
        if (!Rast_is_d_null_value(&values[i]))
            count++;

    *result = count;
}

<<<<<<< HEAD
void w_count(DCELL *result, DCELL (*values)[2], int n,
             const void *closure UNUSED)
=======
void w_count(DCELL *result, DCELL (*values)[2], int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    DCELL count;
    int i;

    count = 0.0;

    for (i = 0; i < n; i++)
        if (!Rast_is_d_null_value(&values[i][0]))
            count += values[i][1];

    *result = count;
}
