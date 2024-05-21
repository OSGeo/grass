#include <grass/gis.h>
#include <grass/raster.h>

<<<<<<< HEAD
<<<<<<< HEAD
void c_max(DCELL *result, DCELL *values, int n, const void *closure)
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
void c_max(DCELL *result, DCELL *values, int n, const void *closure UNUSED)
=======
void c_max(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void c_max(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
{
    DCELL max;
    int i;

    Rast_set_d_null_value(&max, 1);

    for (i = 0; i < n; i++) {
        if (Rast_is_d_null_value(&values[i]))
            continue;

        if (Rast_is_d_null_value(&max) || max < values[i])
            max = values[i];
    }

    if (Rast_is_d_null_value(&max))
        Rast_set_d_null_value(result, 1);
    else
        *result = max;
}

<<<<<<< HEAD
<<<<<<< HEAD
void w_max(DCELL *result, DCELL (*values)[2], int n, const void *closure)
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
void w_max(DCELL *result, DCELL (*values)[2], int n, const void *closure UNUSED)
=======
void w_max(DCELL *result, DCELL (*values)[2], int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void w_max(DCELL *result, DCELL (*values)[2], int n, const void *closure)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
{
    DCELL max;
    int i;

    Rast_set_d_null_value(&max, 1);

    for (i = 0; i < n; i++) {
        if (Rast_is_d_null_value(&values[i][0]))
            continue;

        if (Rast_is_d_null_value(&max) || max < values[i][0])
            max = values[i][0];
    }

    if (Rast_is_d_null_value(&max))
        Rast_set_d_null_value(result, 1);
    else
        *result = max;
}
