
/**
 * \file V_const.c
 *
 * \brief Display constant functions.
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

#include <stdio.h>
#include <grass/vask.h>


/**
 * \fn int V_const (void *src, char var_type, int row, int col, int length)
 *
 * \brief Define screen constant.
 *
 * This routine allows a program to identify a constant, and where that 
 * constant should be placed on the screen in the next call to 
 * <i>V_call()</i>. <b>var_type</b> is one of <i>int</i>, <i>long</i>, 
 * <i>float</i>, <i>double</i>, or <i>char</i>.
 *
 * \param[in] src
 * \param[in] var_type
 * \param[in] row
 * \param[in] col
 * \param[in] len
 * \return 0 on success
 * \return -1 on error
 */

int V_const(void *src, int var_type, int row, int col, int length)
{
    union target targetptr;

    targetptr.i = src;

    if (V__.NUM_CONST >= MAX_CONST) {
	V_error("Too many constants in call to V_const");
	return (-1);
    }
    if ((row < 0) || (row >= MAX_LINE)) {
	V_error("Illegal row (%d) in call to V_const", row);
	return (-1);
    }
    if ((col < 0) || (col > 80)) {
	V_error("Illegal column (%d) in call to V_const", col);
	return (-1);
    }
    if ((length < 0) || ((length + col) > 80)) {
	V_error("Length out of bounds in call to V_const");
	return (-1);
    }

    if ((var_type == 's') || (var_type == 'i') || (var_type == 'f')
	|| (var_type == 'l') || (var_type == 'd')) {
	V__.constant[V__.NUM_CONST].targetptr = targetptr;
	V__.constant[V__.NUM_CONST].var_type = var_type;
	V__.constant[V__.NUM_CONST].row = row;
	V__.constant[V__.NUM_CONST].col = col;
	V__.constant[V__.NUM_CONST].length = length;
	V__.constant[V__.NUM_CONST].decimal_places = V__.decimal_places;

	V__.NUM_CONST++;
	return (0);
    }
    else {
	V_error("Illegal variable type in call to V_const");
	return (-1);
    }
}
