/*!
   \file lib/vector/Vlib/rewind.c

   \brief Vector library - rewind data (native format)

   Higher level functions for reading/writing/manipulating vectors.

   SPDX-FileCopyrightText: 2001-2009, 2011-2012 Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 */

#include <grass/vector.h>

/*! \brief Rewind vector map to cause reads to start at beginning on
   non-topological level (level 1) - native format - internal use only

   \param Map pointer to Map_info struct

   \return 0 on success
   \return -1 on error
 */
int V1_rewind_nat(struct Map_info *Map)
{
    return dig_fseek(&(Map->dig_fp), Map->head.head_size, SEEK_SET);
}

/*! \brief Rewind vector map to cause reads to start at beginning on
   topological level (level 2) - native format - internal use only

   \param Map pointer to Map_info struct

   \return 0 on success
   \return -1 on error
 */
int V2_rewind_nat(struct Map_info *Map)
{
    Map->next_line = 1;
    return V1_rewind_nat(Map); /* make sure level 1 reads are reset too */
}
