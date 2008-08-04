#include "psdriver.h"

void PS_Line_width(int width)
{
    if (width < 0)
	width = 0;

    output("%d WIDTH\n", width);
}
