#include "global.h"

int report(void)
{
    int unit1, unit2;

    if (stats_flag == STATS_ONLY)
	return 1;

    if (nunits == 0)
	print_report(0, -1);
    else
	for (unit1 = 0; unit1 < nunits; unit1 = unit2 + 1) {
	    unit2 = unit1 + 2;
	    if (unit2 >= nunits)
		unit2 = nunits - 1;
	    print_report(unit1, unit2);
	}

    return 0;
}
