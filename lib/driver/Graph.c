#include "driver.h"
#include "driverlib.h"

int COM_Graph_set(int argc, char **argv)
{
    if (driver->Graph_set)
	return (*driver->Graph_set) (argc, argv);
    return 0;
}

void COM_Graph_close(void)
{
    if (driver->Graph_close)
	(*driver->Graph_close) ();
}
