// SPDX-License-Identifier: GPL-2.0-or-later
#include <grass/dbmi.h>
#include <grass/dbstubs.h>

int db__driver_create_index(dbIndex *index G_UNUSED)
{
    return DB_OK;
}
