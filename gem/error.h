
/***************************************************************************
 *            tools.h
 *
 *  Mon Apr 18 15:04:11 2005
 *  Copyright  2005  Benjamin Ducke
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _ERROR_H
#define _ERROR_H

void print_error(int err_code, char *msg, ...)
    __attribute__ ((format(printf, 2, 3)));

void print_warning(char *msg, ...) __attribute__ ((format(printf, 1, 2)));

void print_done(void);

#endif /* _ERROR_H */
