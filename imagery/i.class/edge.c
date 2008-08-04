#include "globals.h"
#include "local_proto.h"


int edge(int x0, int y0, int x1, int y1)
{
    float m;
    float x;

    if (y0 == y1)
	return 0;

    x = x0;
    m = (float)(x0 - x1) / (float)(y0 - y1);

    if (y0 < y1)
	while (++y0 < y1) {
	    x0 = (x += m) + .5;
	    edge_point(x0, y0);
	}
    else
	while (--y0 > y1) {
	    x0 = (x -= m) + .5;
	    edge_point(x0, y0);
	}

    return 0;
}
