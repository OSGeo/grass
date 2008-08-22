
#include "pngdriver.h"

void PNG_Respond(void)
{
    if (png.auto_write)
	write_image();
}
