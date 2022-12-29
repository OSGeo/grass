#include "driver.h"
#include "driverlib.h"

void COM_Text(const char *text)
{
    switch (font_get_type()) {
    case GFONT_STROKE:
	soft_text(text);
	break;
    case GFONT_FREETYPE:
	soft_text_freetype(text);
	break;
    case GFONT_DRIVER:
	if (driver->Text)
	    (*driver->Text)(text);
	break;
    }
}

