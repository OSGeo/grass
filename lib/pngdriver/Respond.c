
#include "pngdriver.h"

void PNG_Respond(void)
{
    if (auto_write)
	write_image();
}
