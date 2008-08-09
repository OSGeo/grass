#include <math.h>
#include "pngdriver.h"

int linewidth;

void PNG_Line_width(double width)
{
    linewidth = (width < 0 ? 0 : (int) floor(width + 0.5));
}
