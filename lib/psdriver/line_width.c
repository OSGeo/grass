#include "psdriver.h"

void PS_Line_width(double width)
{
    if (width < 0)
	width = 0;

    output("%f WIDTH\n", width);
}
