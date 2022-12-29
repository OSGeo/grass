#include "psdriver.h"

void PS_Erase(void)
{
    if (ps.encapsulated)
	output("%d %d %d %d BOX\n", ps.left, ps.top, ps.right, ps.bot);
    else
	output("ERASE\n");
}
