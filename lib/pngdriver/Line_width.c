#include "pngdriver.h"

int linewidth;

void PNG_Line_width(int width)
{
    linewidth = (width < 0 ? 0 : width);
}
