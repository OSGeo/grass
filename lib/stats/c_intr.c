#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/stats.h>

<<<<<<< HEAD
void c_intr(DCELL *result, DCELL *values, int n, const void *closure)
=======
<<<<<<< HEAD
<<<<<<< HEAD
void c_intr(DCELL *result, DCELL *values, int n, const void *closure UNUSED)
=======
void c_intr(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void c_intr(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
{
    DCELL center;
    int count;
    int diff;
    int i;

    if (Rast_is_d_null_value(&values[n / 2])) {
        Rast_set_d_null_value(result, 1);
        return;
    }

    center = values[n / 2];
    count = 0;
    diff = 0;

    for (i = 0; i < n; i++) {
        if (Rast_is_d_null_value(&values[i]))
            continue;

        count++;
        if (values[i] != center)
            diff++;
    }

    count--;

    if (count <= 0)
        *result = 0;
    else
        *result = (diff * 100.0 + (count / 2)) / count + 1;
}
