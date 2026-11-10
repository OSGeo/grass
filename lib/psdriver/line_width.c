// SPDX-License-Identifier: GPL-2.0-or-later
#include "psdriver.h"

void PS_Line_width(double width)
{
    if (width < 0)
        width = 0;

    output("%f WIDTH\n", width);
}
