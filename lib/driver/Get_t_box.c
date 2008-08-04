#include "driver.h"
#include "driverlib.h"

void COM_Get_text_box(const char *text, int *t, int *b, int *l, int *r)
{
    if (!font_is_freetype()) {
	soft_text_ext(cur_x, cur_y,
		      text_size_x, text_size_y, text_rotation, text);
	get_text_ext(t, b, l, r);
    }
    else {
	soft_text_ext_freetype(cur_x, cur_y,
			       text_size_x, text_size_y, text_rotation, text);
	get_text_ext_freetype(t, b, l, r);
    }
}
