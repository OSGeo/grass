#include <math.h>

#include <grass/arraystats.h>
#include <grass/gis.h>

/*provides basic univar stats */
void AS_basic_stats(const double data[], int count, struct GASTATS *stats)
{
    int i = 1;
    double sum = 0, sumsq = 0, sumabs = 0;
    double dev = 0, dev2 = 0;

    stats->count = count;
    stats->min = data[0];
    stats->max = data[count - 1];

    for (i = 0; i < count; i++) {
        sum += data[i];
        sumabs += fabs(data[i]);
        sumsq += data[i] * data[i];
    }
    stats->sum = sum;
    stats->sumabs = sumabs;
    stats->sumsq = sumsq;

    stats->mean = stats->sum / stats->count;
    stats->meanabs = stats->sumabs / stats->count;
    for (i = 0; i < count; i++) {
        dev2 = dev2 + (data[i] - stats->mean) * (data[i] - stats->mean);
        dev = dev + (data[i] - stats->mean);
    }

    stats->var = (dev2 - (dev * dev / stats->count)) / stats->count;
    stats->stdev = sqrt(stats->var);

    return;
}

void AS_eqdrt(double vectx[], double vecty[], int i1, int i2, double *a,
              double *b, double *c)
{
    double x1 = vectx[i1];
    double y1 = vecty[i1];
    double x2 = vectx[i2];
    double y2 = vecty[i2];

    if (i1 == 0) {
        x1 = 0.0;
        y1 = 0.0;
    }
    else {
        x1 = vectx[i1];
        y1 = vecty[i1];
    }

    *a = 0.0;
    *b = 0.0;
    *c = 0.0;

    double bn = y1 - y2;
    double bd = x1 - x2;

    if (fabs(bd) < GRASS_EPSILON) {
        if (fabs(bn) < GRASS_EPSILON) {
            G_debug(3, "Points are equal\n");
        }
        else {
            *c = x1;
        }
    }
    else {
        *b = bn / bd;
        *a = y1 - *b * x1;
    }
}
