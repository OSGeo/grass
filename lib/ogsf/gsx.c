/*!
   \file lib/ogsf/gsx.c

   \brief OGSF library - loading and manipulating surfaces

   GRASS OpenGL gsurf OGSF Library

   SPDX-FileCopyrightText: 1999-2008 Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Bill Brown USACERL (December 1993)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <grass/ogsf.h>

void (*Cxl_func)(void);

static int Cxl = 0;

/*!
   \brief Check for cancel

   \return code
 */
int GS_check_cancel(void)
{
    Cxl_func();

    return (Cxl);
}

/*!
   \brief Set cancel
 */
void GS_set_cancel(int c)
{
    Cxl = c;

    return;
}

/*!
   \brief Set cxl function

   \param f pointer to function
 */
void GS_set_cxl_func(void (*f)(void))
{
    Cxl_func = f;

    return;
}
