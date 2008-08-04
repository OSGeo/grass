
#include "cairodriver.h"

void Cairo_Respond(void)
{
    if (auto_write)
	write_image();
}
