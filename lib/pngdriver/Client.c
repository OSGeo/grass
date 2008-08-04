
#include "pngdriver.h"

void PNG_Client_Close(void)
{
    if (auto_write)
	write_image();
}
