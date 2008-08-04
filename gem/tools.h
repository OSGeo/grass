
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

#ifndef _TOOLS_H
#define _TOOLS_H

char *basename(char *path);

void mkdir_s(char *pathname, char *mode);

int chop(char *string);

int insert_str(char *str, int pos, char **strarr);

int delete_str(int pos, char **strarr);

int find_pos(char *str, char **strarr, int start);

void dump_str(FILE * f, char **strarr);

void get_package_name(char *path, char *name);

char *nc_fgets(char *s, int size, FILE * stream);

char *nc_fgets_nb(char *s, int size, FILE * stream);

char *nc_fgets_html(char *s, int size, FILE * stream);

void dump_ascii(char *file, char *heading);

void dump_plain(char *file, char *tmpfile);

void dump_html(char *file, char *tmpfile);

void list_binaries(char *package);

int binaries_exist(char *package, char *binaries);

int check_filetype(char *file);

void wget_extension(char *url);

void su(char *gisbase, char *cmd);

int vercmp(int major, int minor, int revision, int major2, int minor2,
	   int revision2);

#endif /* _TOOLS_H */
