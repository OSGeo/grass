#include <math.h>
#include "pngdriver.h"

void PNG_Line_width(double width)
{
    png.linewidth = (width < 0 ? 0 : (int) floor(width + 0.5));
}
