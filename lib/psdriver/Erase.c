#include "psdriver.h"

void PS_Erase(void)
{
    if (encapsulated)
	output("%d %d %d %d BOX\n", screen_left, screen_top, screen_right,
	       screen_bottom);
    else
	output("ERASE\n");
}
