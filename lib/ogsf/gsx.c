/*!
   \file lib/ogsf/gsx.c

   \brief OGSF library - loading and manipulating surfaces

   GRASS OpenGL gsurf OGSF Library

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the
   GNU General Public License (>=v2).
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL (December 1993)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <grass/ogsf.h>

<<<<<<< HEAD
<<<<<<< HEAD
void (*Cxl_func)(void);
=======
void (*Cxl_func)();
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void (*Cxl_func)();
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

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

   \param pointer to function
 */
void GS_set_cxl_func(void (*f)(void))
{
    Cxl_func = f;

    return;
}
