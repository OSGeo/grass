#include "driver.h"
#include "driverlib.h"

void COM_Get_text_box(const char *text, double *t, double *b, double *l, double *r)
{
    if (driver->Text_box) {
	(*driver->Text_box)(text, t, b, l, r);
	return;
    }

    if (!font_is_freetype())
	get_text_ext(text, t, b, l, r);
    else
	get_text_ext_freetype(text, t, b, l, r);
}

