#include <grass/gis.h>
#include <grass/stats.h>

<<<<<<< HEAD
void c_divr(DCELL *result, DCELL *values, int n, const void *closure UNUSED)
=======
void c_divr(DCELL *result, DCELL *values, int n, const void *closure)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    int count;
    DCELL prev;
    int i;

    /* sort the array of values, then count differences */

    n = sort_cell(values, n);

    if (n == 0) {
        *result = 0;
        return;
    }

    count = 1;
    prev = values[0];

    for (i = 0; i < n; i++)
        if (values[i] != prev) {
            prev = values[i];
            count++;
        }

    *result = (DCELL)count;
}
