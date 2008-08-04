
/***************************************************************************
 *            reg_entries.h
 *
 *  Fri May 13 11:35:46 2005
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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _REG_ENTRIES_H
#define _REG_ENTRIES_H

void register_entries_gisman(char *pkg_short_name, char *gisbase);

void register_entries_gisman2(char *pkg_short_name, char *gisbase);

int deregister_entries_gisman(char *pkg_short_name, char *gisbase);

void deregister_entries_gisman2(char *pkg_short_name, char *gisbase);

int restore_entries_gisman(char *gisbase);

#endif /* _REG_ENTRIES_H */
