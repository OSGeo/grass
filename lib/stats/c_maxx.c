#include <grass/gis.h>
#include <grass/raster.h>

<<<<<<< HEAD
<<<<<<< HEAD
void c_maxx(DCELL *result, DCELL *values, int n, const void *closure UNUSED)
=======
void c_maxx(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void c_maxx(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
{
    DCELL max, maxx;
    int i;

    Rast_set_d_null_value(&max, 1);
    Rast_set_d_null_value(&maxx, 1);

    for (i = 0; i < n; i++) {
        if (Rast_is_d_null_value(&values[i]))
            continue;

        if (Rast_is_d_null_value(&max) || max < values[i]) {
            max = values[i];
            maxx = i;
        }
    }

    if (Rast_is_d_null_value(&maxx))
        Rast_set_d_null_value(result, 1);
    else
        *result = maxx;
}
