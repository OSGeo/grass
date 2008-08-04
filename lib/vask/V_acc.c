
/**
 * \file V_acc.c
 *
 * \brief Decimal precision functions.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <grass/vask.h>


/**
 * \brief Set number of decimal places.
 * 
 * <i>V_float_accuracy()</i> defines the number of decimal places in 
 * which floats and doubles are displayed or accepted. <b>Num</b> is an 
 * integer value defining the number of decimal places to be used. This 
 * routine affects subsequent calls to <i>V_const()</i> and 
 * <i>V_ques()</i>. Various inputs or displayed constants can be
 * represented with different numbers of decimal places within the same 
 * screen display by making different calls to <i>V_float_accuracy()</i> 
 * before calls to <i>V_ques()</i> or <i>V_const()</i>. <i>V_clear()</i> 
 * resets the number of decimal places to the default (which is 
 * unlimited).
 *
 *  \param[in] n
 *  \return always returns 0
 */

void V_float_accuracy(int n)
{
    V__.decimal_places = n;
}
