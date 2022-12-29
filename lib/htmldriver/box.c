#include "htmlmap.h"

void HTML_Box(double x1, double y1, double x2, double y2)
{
    HTML_Begin();
    HTML_Move(x1, y1);
    HTML_Cont(x1, y2);
    HTML_Cont(x2, y2);
    HTML_Cont(x2, y1);
    HTML_Fill();
}

