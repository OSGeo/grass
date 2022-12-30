#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/stats.h>

void c_quant(DCELL *result, DCELL *values, int n, const void *closure)
{
    double quant = *(const double *)closure;
    double k;
    int i0, i1;

    n = sort_cell(values, n);

    if (n < 1) {
        Rast_set_d_null_value(result, 1);
        return;
    }

    /* algorithm type 7 of Hyndman and Fan (1996), default in R */
    k = quant * (n - 1);
    i0 = (int)floor(k);
    i1 = (int)ceil(k);

    *result =
        (i0 == i1) ? values[i0] : values[i0] * (i1 - k) + values[i1] * (k - i0);
}

<<<<<<< HEAD
void c_quart1(DCELL *result, DCELL *values, int n, const void *closure UNUSED)
=======
void c_quart1(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    static const double q = 0.25;

    c_quant(result, values, n, &q);
}

<<<<<<< HEAD
void c_quart3(DCELL *result, DCELL *values, int n, const void *closure UNUSED)
=======
void c_quart3(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    static const double q = 0.75;

    c_quant(result, values, n, &q);
}

<<<<<<< HEAD
void c_perc90(DCELL *result, DCELL *values, int n, const void *closure UNUSED)
=======
void c_perc90(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    static const double q = 0.90;

    c_quant(result, values, n, &q);
}

void w_quant(DCELL *result, DCELL (*values)[2], int n, const void *closure)
{
    double quant = *(const double *)closure;
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
        if (k >= total * quant)
            break;
    }

    *result = values[i][0];
}

<<<<<<< HEAD
void w_quart1(DCELL *result, DCELL (*values)[2], int n,
              const void *closure UNUSED)
=======
void w_quart1(DCELL *result, DCELL (*values)[2], int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    static const double q = 0.25;

    w_quant(result, values, n, &q);
}

<<<<<<< HEAD
void w_quart3(DCELL *result, DCELL (*values)[2], int n,
              const void *closure UNUSED)
=======
void w_quart3(DCELL *result, DCELL (*values)[2], int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    static const double q = 0.75;

    w_quant(result, values, n, &q);
}

<<<<<<< HEAD
void w_perc90(DCELL *result, DCELL (*values)[2], int n,
              const void *closure UNUSED)
=======
void w_perc90(DCELL *result, DCELL (*values)[2], int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    static const double q = 0.90;

    w_quant(result, values, n, &q);
}
