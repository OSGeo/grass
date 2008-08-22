
#include "cairodriver.h"

void Cairo_Respond(void)
{
    if (ca.auto_write)
	cairo_write_image();
}
