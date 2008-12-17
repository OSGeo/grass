#include "driver.h"
#include "driverlib.h"

void COM_Text(const char *text)
{
    if (driver->Text) {
	(*driver->Text)(text);
	return;
    }

    if (!font_is_freetype())
	soft_text(text);
    else
	soft_text_freetype(text);
}

