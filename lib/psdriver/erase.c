#include "psdriver.h"

void PS_Erase(void)
{
    if (ps.encapsulated)
        output("%lf %lf %lf %lf BOX\n", ps.left, ps.top, ps.right, ps.bot);
    else
        output("ERASE\n");
}
