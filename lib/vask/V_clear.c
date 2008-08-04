
/**
 * \file V_clear.c
 *
 * \brief Screen clearning functions.
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


#define DECIMAL_PLACES -1


/*!
 * \brief Zero out prompt and answer arrays.
 *
 * This routine initializes the screen description information and must 
 * be called before each new screen layout description.
 *
 *  \return always returns 0
 */

void V_clear(void)
{
    static const char text[] = "";
    int at_answer;

    for (at_answer = 0; at_answer < MAX_ANSW; at_answer++)
	V__.usr_answ[at_answer].length = 0;

    for (at_answer = 0; at_answer < MAX_CONST; at_answer++)
	V__.constant[at_answer].length = 0;

    for (at_answer = 0; at_answer < MAX_LINE; at_answer++)
	V__.page.line[at_answer] = text;

    V__.NUM_CONST = 0;
    V__.NUM_ANSW = 0;
    V__.NUM_LINE = 0;
    V_float_accuracy(DECIMAL_PLACES);
    sprintf(V__.interrupt_msg, "CANCEL");
}
