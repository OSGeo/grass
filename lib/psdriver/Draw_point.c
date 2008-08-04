#include "psdriver.h"

void PS_draw_point(int x, int y)
{
    output("%d %d POINT\n", x, y);
}
