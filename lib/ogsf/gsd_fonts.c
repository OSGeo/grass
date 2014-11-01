/*!
   \file lib/ogsf/gsd_fonts.c

   \brief OGSF library - loading and manipulating surfaces

   GRASS OpenGL gsurf OGSF Library 

   \todo This file needs to be re-written in OpenGL

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL, GMSL/University of Illinois
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <string.h>
#include <assert.h>

#include <grass/ogsf.h>

#include "rgbpack.h"

/*!
   \brief Get text width

   \param s text string
   \param size size

   \return text width
 */
int gsd_get_txtwidth(const char *s, int size)
{
    int width, len;

    len = strlen(s);
    width = (size * len) / 2;

    return (width);
}

/*!
   \brief Get text  height

   \param size size

   \return text height
 */
int gsd_get_txtheight(int size)
{
    unsigned long height;

    height = size / 2;

    return (height);

}

/*!
   \brief Get text descender

   yorig ?? 

   Is this defined somewhere ?

   \return 2
 */
int get_txtdescender(void)
{
    return (2);
}

/*!
   \brief Get text offset

   xorig ??

   Is this defined somewhere ?

   \return 0
 */
int get_txtxoffset(void)
{
    return (0);
}

/*!
   \brief Display label

   \param fontbase font-base
   \param lab_pos label position
   \param txt text string
 */
void do_label_display(GLuint fontbase, float *lab_pos, const char *txt)
{
    glRasterPos2f(lab_pos[X], lab_pos[Y]);
    glListBase(fontbase);
    glCallLists(strlen(txt), GL_UNSIGNED_BYTE, (const GLvoid *)txt);

    return;
}
