#include "driver.h"
#include "driverlib.h"

void COM_Get_text_box(const char *text, double *t, double *b, double *l, double *r)
{
    switch (font_get_type()) {
    case GFONT_STROKE:
	get_text_ext(text, t, b, l, r);
	break;
    case GFONT_FREETYPE:
	get_text_ext_freetype(text, t, b, l, r);
	break;
    case GFONT_DRIVER:
	if (driver->Text_box)
	    (*driver->Text_box)(text, t, b, l, r);
	break;
    }
}

