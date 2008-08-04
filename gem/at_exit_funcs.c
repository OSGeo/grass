
/***************************************************************************
 *            at_exit_funcs.c
 *
 *  Mon Apr 18 14:52:20 2005
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


/* unset environment variables */
void exit_env(void)
{

    /*
       NOT NECESSARY, as process cannot set env vars of caller anyway and this
       gives trouble with MINGW compilation, too.
     */

    /*
       unsetenv ("GINSTALL_DST");
       unsetenv ("GINSTALL_INC");
       unsetenv ("GINSTALL_LIB");   
       unsetenv ("UNINSTALL_BASE");
       unsetenv ("GEM_EXT_NAME");
       unsetenv ("GEM_EXT_VERSION");                
       unsetenv ("GEM_EXT_DESCR");
       unsetenv ("GEM_EXT_INFO");
       unsetenv ("GEM_EXT_DEPS");   
       unsetenv ("GEM_EXT_BUGS");
       unsetenv ("GEM_EXT_AUTHORS");
       unsetenv ("GEM_GRASS_DIR");
       unsetenv ("GEM_ACTION");
       unsetenv ("INSTALL_BASE");
       unsetenv ("INSTALL_TYPE");
       unsetenv ("GEM_FORCE");
       unsetenv ("GEM_VERBOSE");
       unsetenv ("GEM_GUI");
       unsetenv ("GEM_C_OPTS");
       unsetenv ("EXT_BASE");
     */
}

/* delete temp directory */
void exit_tmp(void)
{
    int error;
    char tmp[MAXSTR];

    DIR *dir;

    /* if TMPDIR is not set: do not call rmdir! */
    if (!strcmp(TMPDIR, "")) {
	TMPCLEAN = 1;
	return;
    }

    if (TMPCLEAN == 0) {	/* a dirty trick to make sure this only runs once */

	/* step out of temporary dir, in case this extension has been */
	/* installed from an archived dir */
	chdir(CWD);

	sprintf(tmp, "rm -rf %s/*", TMPDIR);

	if (VERBOSE) {
	    fprintf(stdout, "Removing temporary extension files...");
	}
	error = system(tmp);

	sprintf(tmp, "rmdir %s", TMPDIR);
	error = system(tmp);

	/* check if extension dir still exists and if so: warn */
	dir = opendir(TMPDIR);
	if (dir != NULL) {
	    print_warning
		("could not remove temporary directory %s.\nPlease remove manually.\n",
		 TMPDIR);
	}
	if (VERBOSE) {
	    print_done();
	}
	TMPCLEAN = 1;
    }
}

/* delete temp database */
void exit_db(void)
{
    int error;
    char tmp[MAXSTR];

    if (TMPDBCLEAN == 0) {	/* a dirty trick to make sure this only runs once */

	/* step out of temporary dir, in case this extension has been */
	/* installed from an archived dir */
	chdir(CWD);

	if (VERBOSE) {
	    fprintf(stdout, "Removing temporary registration files...");
	}

	if (strcmp(TMPDB, "")) {
	    sprintf(tmp, "rm -rf %s", TMPDB);
	    error = system(tmp);
	    if (error != 0) {
		print_warning
		    ("could not remove temporary file %s.\nPlease remove manually.\n",
		     TMPDB);
	    }
	}

	if (strcmp(TMP_GISMAN, "")) {
	    sprintf(tmp, "rm -f %s", TMP_GISMAN);
	    error = system(tmp);
	    if (error != 0) {
		print_warning
		    ("could not remove temporary file %s.\nPlease remove manually.\n",
		     TMP_GISMAN);
	    }
	}

	if (strcmp(TMP_DESCR, "")) {
	    sprintf(tmp, "rm -f %s", TMP_DESCR);
	    error = system(tmp);
	    if (error != 0) {
		print_warning
		    ("could not remove temporary file %s.\nPlease remove manually.\n",
		     TMP_DESCR);
	    }
	}

	if (strcmp(TMP_INFO, "")) {
	    sprintf(tmp, "rm -f %s", TMP_INFO);
	    error = system(tmp);
	    if (error != 0) {
		print_warning
		    ("could not remove temporary file %s.\nPlease remove manually.\n",
		     TMP_INFO);
	    }
	}

	if (strcmp(TMP_DEPS, "")) {
	    sprintf(tmp, "rm -f %s", TMP_DEPS);
	    error = system(tmp);
	    if (error != 0) {
		print_warning
		    ("could not remove temporary file %s.\nPlease remove manually.\n",
		     TMP_INFO);
	    }
	}

	if (strcmp(TMP_BUGS, "")) {
	    sprintf(tmp, "rm -f %s", TMP_BUGS);
	    error = system(tmp);
	    if (error != 0) {
		print_warning
		    ("could not remove temporary file %s.\nPlease remove manually.\n",
		     TMP_INFO);
	    }
	}

	if (strcmp(TMP_AUTHORS, "")) {
	    sprintf(tmp, "rm -f %s", TMP_AUTHORS);
	    error = system(tmp);
	    if (error != 0) {
		print_warning
		    ("could not remove temporary file %s.\nPlease remove manually.\n",
		     TMP_INFO);
	    }
	}

	if (strcmp(TMP_HTML, "")) {
	    sprintf(tmp, "rm -f %s", TMP_HTML);
	    error = system(tmp);
	    if (error != 0) {
		print_warning
		    ("could not remove temporary file %s.\nPlease remove manually.\n",
		     TMP_HTML);
	    }
	}

	if (!VERBOSE) {
	    if (strcmp(TMP_NULL, "")) {
		sprintf(tmp, "rm -f %s", TMP_NULL);
		error = system(tmp);
		if (error != 0) {
		    print_warning
			("could not remove temporary file %s.\nPlease remove manually.\n",
			 TMP_NULL);
		}
	    }
	}

	if (VERBOSE) {
	    print_done();
	}
	TMPDBCLEAN = 1;
    }
}

/* show a message at end of program */
void exit_msg(void)
{
    if (ERROR < 0) {
	fprintf(stdout,
		"Program exited with an error (code %i). Operation aborted.\n",
		ERROR);
    }
    else {
	if (WARNINGS == 1) {
	    fprintf(stdout,
		    "Job done but there was one warning. Please check.\n");
	}
	if (WARNINGS > 1) {
	    fprintf(stdout,
		    "Job done but there were %i warnings. Please check.\n",
		    WARNINGS);
	}
    }
}
