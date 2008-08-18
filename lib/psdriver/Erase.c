#include "psdriver.h"

void PS_Erase(void)
{
    if (encapsulated)
	output("%d %d %d %d BOX\n", left, top, right, bot);
    else
	output("ERASE\n");
}
