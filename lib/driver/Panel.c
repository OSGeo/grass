#include "driver.h"
#include "driverlib.h"

void COM_Panel_save(const char *name, int top, int bottom, int left,
		    int right)
{
    if (driver->Panel_save)
	(*driver->Panel_save) (name, top, bottom, left, right);
}

void COM_Panel_restore(const char *name)
{
    if (driver->Panel_restore)
	(*driver->Panel_restore) (name);
}

void COM_Panel_delete(const char *name)
{
    if (driver->Panel_delete)
	(*driver->Panel_delete) (name);
}
