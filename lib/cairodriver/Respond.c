
#include "cairodriver.h"

void Cairo_Respond(void)
{
    if (auto_write)
	cairo_write_image();
}
