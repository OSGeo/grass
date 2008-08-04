
/***************************************************************************
 *            reg_deps.c
 *
 *  Mon Apr 18 15:19:04 2005
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

#include "globals.h"


/*
   Reads in the "depends" file and returns a string of comma-separated
   extensions, on which this one depends.
 */
char *depstr(char *package, char *gisbase)
{
    char file[MAXSTR];
    char line[MAXSTR];
    char *str;
    int first;
    int error;
    FILE *f_deps;

    char short_name[MAXSTR];

    /* check if 'depends' exists and is readable */
    sprintf(file, "../depends");
    f_deps = fopen(file, "r");
    if (f_deps == NULL) {
	if (errno == ENOENT) {
	    /* file does not exist, return an empty string */
	    return ("");
	}
	else {
	    /* sth. strange happened */
	    print_error(ERR_CHECK_DEPS, "checking for file '%s': %s\n", file,
			strerror(errno));
	}
    }

    first = 1;
    str = malloc(sizeof(char) * MAXSTR);
    while (nc_fgets_nb(line, MAXSTR, f_deps) != NULL) {
	if (strlen(line) > 0) {
	    error = sscanf(line, "%s", short_name);
	    if ((error > 0) && (strcmp("GRASS", short_name))) {
		if (first) {
		    strcat(str, "\t");
		    strcat(str, short_name);
		    first = 0;
		}
		else {
		    strcat(str, ",");
		    strcat(str, short_name);
		}
	    }
	}
    }

    fclose(f_deps);
    return (str);
}

/* 
   Reads in GISBASE/etc/extensions.db 
   Adds a line to the file GISBASE/etc/extensions.db that contains: 
   extension name and version, 'src' or name of binary files.
   Adds extension name to the end of every entry mentioned in 'depends' 
 */
void register_extension(char *gisbase, char *bins, char *pkg_short_name,
			int pkg_major, int pkg_minor, int pkg_revision)
{
    char file[MAXSTR];
    char str[MAXSTR];
    int n_lines;
    int error;
    int db_exists;
    int must_register;
    int copy_thru;
    int ext_exists;
    FILE *f_in, *f_out;

    char short_name[MAXSTR];
    char inst_bins[MAXSTR];
    char deps[MAXSTR];
    int major, minor, revision;


    /* check, if extensions.db exists and is readable */
    db_exists = 1;
    sprintf(file, "%s/etc/extensions.db", gisbase);
    f_in = fopen(file, "r");
    if (f_in == NULL) {
	if (errno == ENOENT) {
	    /* file does not yet exist */
	    db_exists = 0;
	}
	else {
	    /* sth. strange happened */
	    fclose(f_in);
	    print_error(ERR_REGISTER_EXT, "checking for file '%s': %s\n",
			file, strerror(errno));
	}
    }

    if (db_exists) {
	/* create a temporary extensions.db file for write access */
	/* TODO: Do not hardcode temp paths */
	strcpy(TMPDB, "/tmp/grass.extensions.db.XXXXXX");	/* TMPDB is a global variable */
	mkstemp(TMPDB);

	f_out = fopen(TMPDB, "w+");
	if (f_out == NULL) {
	    print_error(ERR_REGISTER_EXT,
			"could not create temp file '%s': %s\n \
							Make sure that directory /tmp exists on your system and you have write permission.\n", TMPDB, strerror(errno));
	}

	atexit(&exit_db);	/* now need to register an at exit func to remove tmpdb automatically! */

	/* count number of extensions.db registry entries */
	n_lines = 0;
	must_register = 1;
	ext_exists = 0;
	while (nc_fgets_nb(str, MAXSTR, f_in) != NULL) {
	    n_lines++;
	    copy_thru = 1;
	    /* read in name and version of registered item */
	    sscanf(str, "%s\t%i.%i.%i\t%s\t%s", short_name, &major, &minor,
		   &revision, inst_bins, deps);
	    /* check, if extension with same name already exists */
	    if (!strcmp(short_name, pkg_short_name)) {
		/* an extension with this name is already installed */
		ext_exists = 1;
		/* check if it's version is lower, same or higher */
		error =
		    vercmp(major, minor, revision, pkg_major, pkg_minor,
			   pkg_revision);
		if ((!FORCE)) {
		    /* error, if another version is present! */
		    if (error < 0) {
			print_error(ERR_EXISTS_EXT,
				    "Extension '%s' with lower version (%i.%i.%i) already installed. You can use -f to overwrite this version, if you know what you are doing.\n",
				    pkg_short_name, major, minor, revision);
		    }
		    else {
			print_error(ERR_EXISTS_EXT,
				    "Extension '%s' with same or higher version (%i.%i.%i) already installed. You can use -f to overwrite this version, if you know what you are doing.\n",
				    pkg_short_name, major, minor, revision);
		    }
		}
		if (FORCE) {	/* this overrides/implies UPGRADE! */
		    if (error == 0) {
			/* if same version installed: just leave everything untouched */
			must_register = 0;
		    }
		    else {
			/* otherwise, work needs be done! */
			must_register = 1;
			copy_thru = 0;
		    }
		}
	    }

	    /* write line to temporary extension file */
	    if (copy_thru) {	/* just copy thru */
		fprintf(f_out, str);
		fflush(f_out);
	    }
	}

	if (must_register) {
	    if (!ext_exists) {
		/* register brand new extension */
		sprintf(deps, "%s", depstr(pkg_short_name, gisbase));
		fprintf(f_out, "%s\t%i.%i.%i\t%s\t%s\n", pkg_short_name,
			pkg_major, pkg_minor, pkg_revision, bins, deps);
	    }
	    else {
		/* register only new version number and binary source, if appropriate */
		sprintf(deps, "%s", depstr(pkg_short_name, gisbase));
		fprintf(f_out, "%s\t%i.%i.%i\t%s\t%s\n", short_name,
			pkg_major, pkg_minor, pkg_revision, inst_bins, deps);
	    }
	}

	fclose(f_out);
	fclose(f_in);
    }

    if ((n_lines == 0) || (!db_exists)) {
	/* extensions.db file does not yet exist or is empty: just create a new one */
	/* TODO: Do not hardcode temp paths */
	strcpy(TMPDB, "/tmp/grass.extensions.db.XXXXXX");	/* tmpdir is a global variable */
	mkstemp(TMPDB);

	f_out = fopen(TMPDB, "w+");
	if (f_out == NULL) {
	    print_error(ERR_REGISTER_EXT,
			"could not create temp db '%s': %s\n \
					Make sure that directory /tmp exists on your system and you have write permission.\n", file, strerror(errno));
	}
	atexit(&exit_db);	/* now need to register an at exit func to remove tmpdb automatically! */

	/* register brand new extension */
	sprintf(deps, "%s", depstr(pkg_short_name, gisbase));
	fprintf(f_out, "%s\t%i.%i.%i\t%s\t%s\n", pkg_short_name, pkg_major,
		pkg_minor, pkg_revision, bins, deps);
	fclose(f_out);
	return;
    }
}


/*
   Removes an entry from GISBASE/etc/extensions.db
 */
void deregister_extension(char *package, char *pkg_short_name, char *gisbase)
{
    char file[MAXSTR];
    char str[MAXSTR];
    int error;
    int db_exists;
    int copy_thru;
    FILE *f_in, *f_out;
    int found_ext;

    char short_name[MAXSTR];
    char inst_bins[MAXSTR];
    char deps[MAXSTR];
    int major, minor, revision;


    db_exists = 0;
    /* check, if extensions.db exists and is readable */
    sprintf(file, "%s/etc/extensions.db", gisbase);
    f_in = fopen(file, "r");
    if (f_in == NULL) {
	if ((errno == ENOENT) && (!FORCE)) {
	    /* file does not yet exist */
	    fclose(f_in);
	    print_error(ERR_DEREGISTER_EXT,
			"could not deregister: no extensions installed\n");
	}
	else {
	    /* sth. strange happened */
	    if (!FORCE) {
		fclose(f_in);
		print_error(ERR_DEREGISTER_EXT,
			    "checking for file '%s': %s\n", file,
			    strerror(errno));
	    }
	}
    }
    else {
	db_exists = 1;
    }

    if (db_exists) {
	db_exists = 0;
	/* create a temporary extensions.db file for write access */
	/* TODO: Do not hardcode temp paths */
	strcpy(TMPDB, "/tmp/grass.extensions.db.XXXXXX");	/* tmpdir is a global variable */
	mkstemp(TMPDB);

	f_out = fopen(TMPDB, "w+");
	if ((f_out == NULL) && (!FORCE)) {
	    print_error(ERR_DEREGISTER_EXT,
			"could not create temp db '%s': %s\n \
							Make sure that directory /tmp exists on your system and you have write permission.\n", file, strerror(errno));
	}
	else {
	    db_exists = 1;
	    atexit(&exit_db);	/* now need to register an at exit func to remove tmpdb automatically! */
	}
    }

    /* if creation of temp db file succeeded: */
    if (db_exists) {
	/* read every line in extension.db */
	found_ext = 0;
	while (nc_fgets_nb(str, MAXSTR, f_in) != NULL) {
	    /* copy thru, if extension name different from current extension's */
	    error =
		sscanf(str, "%s\t%i.%i.%i\t%s\t%s", short_name, &major,
		       &minor, &revision, inst_bins, deps);
	    if (error > 0) {
		copy_thru = 1;
		if (!strcmp(pkg_short_name, short_name)) {
		    copy_thru = 0;
		    found_ext = 1;
		}
		if (copy_thru) {
		    /* abort, if a dependency on current extension is found */
		    if ((strstr(deps, pkg_short_name)) && (!FORCE)) {
			print_error(ERR_DEREGISTER_EXT,
				    "cannot uninstall extension '%s' it is needed by '%s'.\n",
				    pkg_short_name, short_name);
		    }
		    fprintf(f_out, str);
		}
	    }
	}

	/* was the extension installed at all? */
	if (found_ext == 0) {
	    print_error(ERR_DEREGISTER_EXT,
			"no extension '%s' registered/installed in '%s'.\n",
			pkg_short_name, gisbase);
	}
	/* mind the right sequence for closing these! */
	fclose(f_in);
	fclose(f_out);
    }
}


/*
   checks GRASS version number and whether all extensions listed in 'depends'
   are satisfied (by analysing GISBASE/etc/extensions.db).
   Terminates program, if missing deps; just returns, if "--force" was given.
 */
void check_dependencies(char *package, char *gisbase, char *grass_version)
{
    char file[MAXSTR];
    char str[MAXSTR];
    char dbstr[MAXSTR];
    int error;
    int db_exists;
    int satisfied;
    FILE *f_deps, *f_db;

    char short_name[MAXSTR];
    int major, minor, revision;
    char dep_short_name[MAXSTR];
    int dep_major, dep_minor, dep_revision;

    if (FORCE) {
	return;
    }

    /* check if 'depends' exists and is readable */
    f_deps = fopen("../depends", "r");
    if (f_deps == NULL) {
	if (errno == ENOENT) {
	    /* file does not exist, no need to check for deps */
	    fprintf(stderr, "\n%s/depends ENOENT\n", package);
	    return;
	}
	else {
	    /* sth. strange happened */
	    fclose(f_deps);
	    print_error(ERR_CHECK_DEPS, "checking for file '%s': %s\n", file,
			strerror(errno));
	}
    }

    db_exists = 1;
    /* check if extensions.db exists and is readable */
    sprintf(file, "%s/etc/extensions.db", gisbase);
    f_db = fopen(file, "r");
    if (f_db == NULL) {
	if (errno == ENOENT) {
	    /* file does not exist */
	    db_exists = 0;
	}
	else {
	    /* sth. strange happened */
	    fclose(f_db);
	    print_error(ERR_CHECK_DEPS, "checking for file '%s': %s\n", file,
			strerror(errno));
	}
    }

    satisfied = 0;
    while (nc_fgets_nb(str, MAXSTR, f_deps) != NULL) {
	/* get first line with dependencies info */
	major = 0;
	minor = 0;
	revision = 0;
	dep_major = 0;
	dep_minor = 0;
	dep_revision = 0;
	error =
	    sscanf(str, "%s\t%i.%i.%i", dep_short_name, &dep_major,
		   &dep_minor, &dep_revision);
	if (error > 0) {
	    if (!strcmp(dep_short_name, "GRASS")) {
		/* a GRASS version dependency has been specified */
		sscanf(grass_version, "%i.%i.%i", &major, &minor, &revision);
		if (vercmp
		    (major, minor, revision, dep_major, dep_minor,
		     dep_revision) < 0) {
		    print_error(ERR_MISSING_DEPS,
				"installed version (%s) of GRASS is too low. Required version is %i.%i.%i\n",
				grass_version, dep_major, dep_minor,
				dep_revision);
		}
		satisfied = 1;	/* let's be tolerant */
	    }
	    else {
		/* a dependency on some other extension has been specified ... */
		if (db_exists) {	/* ... if no extensions installed yet, it must be unsatisfied! */
		    satisfied = 0;
		    rewind(f_db);
		    while (nc_fgets_nb(dbstr, MAXSTR, f_db) != NULL) {
			major = 0;
			minor = 0;
			revision = 0;
			error =
			    sscanf(dbstr, "%s\t%i.%i.%i", short_name, &major,
				   &minor, &revision);
			if (error > 0) {
			    if (!strcmp(short_name, dep_short_name)) {
				/* found an installed extension that satisfies name, now check
				   for version number */
				if (vercmp
				    (major, minor, revision, dep_major,
				     dep_minor, dep_revision) < 0) {
				    print_error(ERR_MISSING_DEPS,
						"installed version %i.%i.%i of required extension '%s' is too low.\n \
													Required version is %i.%i.%i\n",
						major, minor, revision, dep_short_name, dep_major, dep_minor,
						dep_revision);
				}
				/* name and version dependencies satisfied */
				satisfied = 1;
			    }
			}
		    }
		    if (!satisfied) {
			print_error(ERR_MISSING_DEPS,
				    "required extension '%s' not found in '%s'.\n",
				    dep_short_name, gisbase);
		    }
		}
	    }
	}
    }
    if (db_exists) {
	fclose(f_db);
    }
    fclose(f_deps);

}
