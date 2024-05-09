#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>

<<<<<<< HEAD
void c_skew(DCELL *result, DCELL *values, int n, const void *closure)
=======
<<<<<<< HEAD
<<<<<<< HEAD
void c_skew(DCELL *result, DCELL *values, int n, const void *closure UNUSED)
=======
void c_skew(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void c_skew(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
{
    DCELL sum, ave, sumsq, sumcb, sdev;
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

    if (count == 0) {
        Rast_set_d_null_value(result, 1);
        return;
    }

    ave = sum / count;

    sumsq = 0;
    sumcb = 0;

    for (i = 0; i < n; i++) {
        DCELL d;

        if (Rast_is_d_null_value(&values[i]))
            continue;

        d = values[i] - ave;
        sumsq += d * d;
        sumcb += d * d * d;
    }

    sdev = sqrt(sumsq / count);

    *result = sumcb / (count * sdev * sdev * sdev);
}

<<<<<<< HEAD
void w_skew(DCELL *result, DCELL (*values)[2], int n, const void *closure)
=======
<<<<<<< HEAD
<<<<<<< HEAD
void w_skew(DCELL *result, DCELL (*values)[2], int n,
            const void *closure UNUSED)
=======
void w_skew(DCELL *result, DCELL (*values)[2], int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void w_skew(DCELL *result, DCELL (*values)[2], int n, const void *closure)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
{
    DCELL sum, ave, sumsq, sumcb, sdev;
    DCELL count;
    int i;

    sum = 0.0;
    count = 0;

    for (i = 0; i < n; i++) {
        if (Rast_is_d_null_value(&values[i][0]))
            continue;

        sum += values[i][0] * values[i][1];
        count += values[i][1];
    }

    if (count == 0) {
        Rast_set_d_null_value(result, 1);
        return;
    }

    ave = sum / count;

    sumsq = 0;
    sumcb = 0;

    for (i = 0; i < n; i++) {
        DCELL d;

        if (Rast_is_d_null_value(&values[i][0]))
            continue;

        d = values[i][0] - ave;
        sumsq += d * d * values[i][1];
        sumcb += d * d * d * values[i][1];
    }

    sdev = sqrt(sumsq / count);

    *result = sumcb / (count * sdev * sdev * sdev);
}
