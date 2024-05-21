#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/stats.h>

<<<<<<< HEAD
<<<<<<< HEAD
void c_median(DCELL *result, DCELL *values, int n, const void *closure)
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
void c_median(DCELL *result, DCELL *values, int n, const void *closure UNUSED)
=======
void c_median(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void c_median(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
{
    n = sort_cell(values, n);

    if (n < 1)
        Rast_set_d_null_value(result, 1);
    else
        *result = (values[(n - 1) / 2] + values[n / 2]) / 2;
}

<<<<<<< HEAD
<<<<<<< HEAD
void w_median(DCELL *result, DCELL (*values)[2], int n, const void *closure)
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
void w_median(DCELL *result, DCELL (*values)[2], int n,
              const void *closure UNUSED)
=======
void w_median(DCELL *result, DCELL (*values)[2], int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void w_median(DCELL *result, DCELL (*values)[2], int n, const void *closure)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
{
    DCELL total;
    int i;
    DCELL k;

    n = sort_cell_w(values, n);

    if (n < 1) {
        Rast_set_d_null_value(result, 1);
        return;
    }

    total = 0.0;
    for (i = 0; i < n; i++)
        total += values[i][1];

    k = 0.0;
    for (i = 0; i < n; i++) {
        k += values[i][1];
        if (k >= total / 2)
            break;
    }

    *result = values[i][0];
}
