#include "stats.h"

long median(struct stats *stats)
{
    double total, sum;
    int i;

    total = 0;
    for (i = 0; i < stats->n; i++)
	total += stats->area[i];

    total /= 2.0;

    sum = 0;
    for (i = 0; i < stats->n; i++) {
	sum += stats->area[i];
	if (sum > total)
	    break;
    }
    if (i == stats->n)
	i--;
    if (i < 0)
	return ((long)0);
    return (stats->cat[i]);
}
