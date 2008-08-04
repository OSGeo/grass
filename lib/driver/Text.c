#include "driver.h"
#include "driverlib.h"

void COM_Text(const char *text)
{
    if (driver->draw_text) {
	(*driver->draw_text) (text);
	return;
    }

    if (!font_is_freetype())
	soft_text(cur_x, cur_y, text_size_x, text_size_y, text_rotation,
		  text);
    else
	soft_text_freetype(cur_x, cur_y, text_size_x, text_size_y,
			   text_rotation, text);
}
