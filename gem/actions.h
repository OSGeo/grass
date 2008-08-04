
/***************************************************************************
 *            actions.h
 *
 *  Mon Apr 18 15:29:34 2005
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

#ifndef _ACTIONS_H
#define _ACTIONS_H

#include "globals.h"

void check_extension(char *package, char *name, int *major, int *minor,
		     int *revision);

void unpack_extension(char *package);

void query_extension(char *package, char *name, int major, int minor,
		     int revision, char *short_name, char *invocation,
		     char *org_name);

void source_install(char *package, char *gisbase, char *pkg_short_name,
		    int pkg_major, int pkg_minor, int pkg_revision,
		    char *grass_version);

void bin_install(char *package, char *gisbase, char *bins,
		 char *pkg_short_name, int pkg_major, int pkg_minor,
		 int pkg_revision, char *grass_version);

void test_install(char *package, char *gisbase, char *pkg_short_name,
		  int pkg_major, int pkg_minor, int pkg_revision,
		  char *grass_version);

void uninstall(char *package, char *pkg_short_name, char *gisbase,
	       char *grass_version);

int source_clean(char *package);

void restore(char *gisbase, char *grass_version);

void list_extensions(char *gisbase);

void run_post(char *package, int action, char *bins, char *gisbase);

#endif /* _ACTIONS_H */
