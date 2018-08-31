#include <math.h>
#include <grass/arraystats.h>


/*provides basic univar stats */
void AS_basic_stats(double *data, int count, struct GASTATS *stats)
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


void AS_eqdrt(double vectx[], double vecty[], int i1, int i2, double *vabc)
{
    double bn = 0, bd = 0, x1 = 0, y1 = 0;

    vabc[0] = 0;
    vabc[1] = 0;
    vabc[2] = 0;
    if (i1 == 0) {
	x1 = 0;
	y1 = 0;
    }
    else {
	x1 = vectx[i1];
	y1 = vecty[i1];
    }
    bn = y1 - vecty[i2];
    bd = x1 - vectx[i2];
    if (bd != 0) {
	vabc[1] = bn / bd;
	vabc[0] = y1 - vabc[1] * x1;
	return;
    }
    if (bn != 0)
	vabc[2] = x1;
    else
	G_debug(3, "Points are equal\n");
    return;
}
