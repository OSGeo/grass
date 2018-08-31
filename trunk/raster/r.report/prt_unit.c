#include "global.h"


int print_unit(int i, int ns, int nl)
{
    char num[50];
    int k;
    double area;

    if (unit[i].type == CELL_COUNTS) {
	sprintf(num, "%*ld", unit[i].len, count_sum(&ns, nl));
    }
    else if (unit[i].type == PERCENT_COVER) {
	k = ns - 1;
	while (k >= 0 && same_cats(k, ns, nl - 1))
	    k--;
	k++;
	area = area_sum(&k, nl - 1);
	if (unit[i].eformat)
	    scient_format(100.0 * area_sum(&ns, nl) / area,
			  num, unit[i].len, unit[i].dp);
	else
	    format_double(100.0 * area_sum(&ns, nl) / area,
			  num, unit[i].len, unit[i].dp);

    }
    else {
	if (unit[i].eformat)
	    scient_format(area_sum(&ns, nl) * unit[i].factor,
			  num, unit[i].len, unit[i].dp);
	else
	    format_double(area_sum(&ns, nl) * unit[i].factor,
			  num, unit[i].len, unit[i].dp);
    }
    fprintf(stdout, "|%s", num);

    return 0;
}
