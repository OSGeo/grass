
/***************************************************************************
 *            reg_html.h
 *
 *  Fri May 20 18:14:46 2005
 *  Copyright  2005  User
 *  Email
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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _REG_HTML_H
#define _REG_HTML_H

void register_html(char *pkg_short_name, char *gisbase, int major, int minor,
		   int revision);

void deregister_html(char *pkg_short_name, char *gisbase);

int restore_html(char *gisbase);

#endif /* _REG_HTML_H */
