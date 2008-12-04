
#include <grass/gis.h>
#include "psdriver.h"

void PS_Color(int r, int g, int b)
{
    if (ps.true_color)
	output("%d %d %d COLOR\n", r, g, b);
    else
	output("%d GRAY\n", (int)(r * 0.299 + g * 0.587 + b * 0.114));
}
