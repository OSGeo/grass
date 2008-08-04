
/***************************************************************************
 *            reg_deps.h
 *
 *  Mon Apr 18 15:23:06 2005
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

#ifndef _REG_DEPS_H
#define _REG_DEPS_H

char *depstr(char *package, char *gisbase);

void register_extension(char *gisbase, char *bins, char *pkg_short_name,
			int pkg_major, int pkg_minor, int pkg_revision);

void deregister_extension(char *package, char *pkg_short_name, char *gisbase);

void check_dependencies(char *package, char *gisbase, char *grass_version);


#endif /* _REG_DEPS_H */
