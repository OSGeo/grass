// SPDX-License-Identifier: GPL-2.0-or-later
#include "driver.h"
#include "driverlib.h"

void COM_Erase(void)
{
    if (driver->Erase)
        (*driver->Erase)();
}
