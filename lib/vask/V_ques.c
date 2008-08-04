
/**
 * \file V_ques.c
 *
 * \brief Display question functions.
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
 * \fn int V_ques (void *src, char var_type, int row, int col, int length)
 *
 * \brief Define screen question.
 *
 * <i>V_ques()</i> allows a program to identify a prompt field and where 
 * that field should be placed on the screen in the next call to 
 * <i>V_call()</i>.<br>
 * <i>V_ques()</i> specifies that the contents of memory at the address 
 * of <b>src</b> are to be displayed on the screen at location 
 * <b>row</b>, <b>col</b> for <b>len</b> characters.<br>
 * <i>V_ques()</i> further specifies that this screen location is a 
 * prompt field. The user will be allowed to change the field on the 
 * screen and thus change the <b>src</b> itself.
 * <b>var_type</b> specifies what type <b>src</b> points to: 'i' (int), 
 * 'l' (long), 'f' (float), 'd' (double), or 's' (character string).<br>
 * <b>row</b> is an integer value of 0-22 specifying the row on the 
 * screen where the value is placed. The top row on the screen is row 
 * 0.<br>
 * <b>col</b> is an integer value of 0-79 specifying the column on the 
 * screen where the value is placed. The leftmost column on the screen 
 * is column 0.<br>
 * <b>len</b> specifies the number of columns that the value will 
 * use.<br>
 * <b>Note:</b> The size of a character array passed to <i>V_ques()</i>
 * must be at least one byte longer than the length of the prompt field 
 * to allow for NULL termination. Currently, you are limited to 20 
 * constants and 80 variables.<br>
 * <b>Warning:</b> These routines store the address of <b>src</b> and 
 * not the value itself. This implies that different variables must be 
 * used for different calls. Programmers will instinctively use 
 * different variables with <i>V_ques()</i>, but it is a stumbling block 
 * for <i>V_const()</i>. Also, the programmer must initialize 
 * <b>src</b> prior to calling these routines.
 *
 * \remarks {Technically <b>src</b> needs to be initialized before
 * the call to <i>V_call()</i> since <i>V_const()</i> and 
 * <i>V_ques()</i> only store the address of <b>src</b>. 
 * <i>V_call()</i> looks up the values and places them on the screen.}
 *
 * \param[in] src
 * \param[in] var_type
 * \param[in] row
 * \param[in] col
 * \param[in] len
 * \return 0 on success
 * \return -1 on error
 */

int V_ques(void *src, int var_type, int row, int col, int length)
{
    union target targetptr;

    targetptr.i = src;

    if (V__.NUM_ANSW >= MAX_ANSW) {
	V_error("Too many questions in call to V_ques");
	return (-1);
    }
    if ((row < 0) || (row >= MAX_LINE)) {
	V_error("Illegal row (%d) in call to V_ques", row);
	return (-1);
    }
    if ((col < 0) || (col >= 80)) {
	V_error("Illegal column (%d) in call to V_ques", col);
	return (-1);
    }
    if (length <= 0) {
	V_error("Negative length in call to V_ques");
	return (-1);
    }
    if (length + col > 80)
	length = 80 - col;

    if ((var_type == 's') || (var_type == 'i') || (var_type == 'f')
	|| (var_type == 'l') || (var_type == 'd')) {
	V__.usr_answ[V__.NUM_ANSW].targetptr = targetptr;
	V__.usr_answ[V__.NUM_ANSW].var_type = var_type;
	V__.usr_answ[V__.NUM_ANSW].row = row;
	V__.usr_answ[V__.NUM_ANSW].col = col;
	V__.usr_answ[V__.NUM_ANSW].length = length;
	V__.usr_answ[V__.NUM_ANSW].decimal_places = V__.decimal_places;

	V__.NUM_ANSW++;
	return (0);
    }
    else {
	V_error("Illegal variable type in call to V_ques");
	return (-1);
    }
}
