#include <stdlib.h>
#include "driver.h"
#include "driverlib.h"

int COM_Graph_set(void)
{
    if (driver->Graph_set)
	return (*driver->Graph_set) ();
    return 0;
}

void COM_Graph_close(void)
{
    if (driver->Graph_close)
	(*driver->Graph_close) ();
}

const char *COM_Graph_get_file(void)
{
    if (driver->Graph_get_file)
        return (*driver->Graph_get_file) ();

    return NULL;
}
