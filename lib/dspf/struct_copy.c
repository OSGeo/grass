// SPDX-FileCopyrightText: 1996-2006 GRASS Development Team
// SPDX-License-Identifier: GPL-2.0-or-later
int struct_copy(char *To, char *From, int size)
{
    for (; size; size--)
        *To++ = *From++;

    return 0;
}
