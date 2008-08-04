
/**
 * \file V_line.c
 *
 * \brief Display line functions.
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
 * \fn int V_line (int linenumber, char *text)
 * 
 * \brief Add line of text to screen for next call to <i>V_call()</i>.
 *
 * This routine is used to place lines of text on the screen. 
 * <b>linenumber</b> is an integer value of 0-22 specifying the row on 
 * the screen where the <b>text</b> is placed. The top row on the screen 
 * is row 0.<br>
 * <b>Warning:</b> <i>V_line()</i> does not copy the text to the screen 
 * description. It only saves the text address. This implies that each 
 * call to <i>V_line()</i> must use a different text buffer.
 *
 * \param[in] linenumber
 * \param[in] text
 * \return 0 on success
 * \return -1 on error
 */

int V_line(int linenumber, const char *text)
{
    if (linenumber >= MAX_LINE || linenumber < 0) {
	V_error("Linenumber out of bounds in call to V_line");
	return (-1);
    }
    V__.page.line[linenumber] = text;

    return 0;
}
