/*!
   \file lib/ogsf/gsget.c

   \brief OGSF library - get map attribute (lower level functions)

   GRASS OpenGL gsurf OGSF Library

   SPDX-FileCopyrightText: 1999-2008 Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Bill Brown USACERL (January 1993)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <grass/ogsf.h>

/*!
   \brief Get map attributes

   \param buff
   \param[in] offset
   \param[out] att

   \return 0 on failure
   \return 1 on success
 */
int get_mapatt(typbuff *buff, int offset, float *att)
{
    if (buff->nm) {
        if (BM_get(buff->nm, (offset % buff->nm->cols),
                   (offset / buff->nm->cols))) {
            return (0);
        }
    }

    *att = (buff->ib   ? (float)buff->ib[offset]
            : buff->sb ? (float)buff->sb[offset]
            : buff->cb ? (float)buff->cb[offset]
            : buff->fb ? (float)buff->fb[offset]
                       : buff->k);

    if (buff->tfunc) {
        *att = (buff->tfunc)(*att, offset);
    }

    return (1);
}
