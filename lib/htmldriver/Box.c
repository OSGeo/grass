#include "htmlmap.h"

void HTML_Box(double x1, double y1, double x2, double y2)
{
    double x[4], y[4];

    x[0] = x1;    y[0] = y1;
    x[1] = x1;    y[1] = y2;
    x[2] = x2;    y[2] = y2;
    x[3] = x2;    y[3] = y1;

    HTML_Polygon(x, y, 4);
}

