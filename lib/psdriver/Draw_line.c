
#include "psdriver.h"

void PS_draw_line(int x1, int y1, int x2, int y2)
{
    output("%d %d %d %d LINE\n", x1, y1, x2, y2);
}
