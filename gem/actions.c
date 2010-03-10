
/***************************************************************************
 *            actions.c
 *
 *  Mon Apr 18 15:25:54 2005
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
#include <fcntl.h>


void check_extension(char *package, char *name, int *major, int *minor,
		     int *revision)
{
    int error;
    char tmp[MAXSTR] = "";
    FILE *f;

    fprintf(stdout, "Checking extension ...");

    sprintf(tmp, "%s", package);
    error = chdir(tmp);
    if (error < 0) {
	print_error(ERR_NO_ACCESS_EXT, "extension '%s' not accessible: %s\n",
		    package, strerror(errno));
    }

    /* get file id */
    f = fopen("id", "r");
    if (f == NULL) {
	print_error(ERR_INVALID_EXT, "'id' file not readable.\n");
    }
    else {
	fscanf(f, "%[<GRASS extension package>] ", tmp);
	if (strcmp("<GRASS extension package>", tmp)) {
	    fclose(f);
	    print_error(ERR_INVALID_EXT, "unknown file identifier.\n");
	}
    }
    fclose(f);

    get_package_name(".", name);

    /* get extension version */
    f = fopen("version", "r");
    if (f == NULL) {
	print_error(ERR_INVALID_EXT, "'version' file not readable.\n");
    }
    else {
	nc_fgets_nb(tmp, MAXSTR, f);
	*minor = 0;
	*revision = 0;
	error = sscanf(tmp, "%i.%i.%i", major, minor, revision);
	if (error < 1) {
	    fclose(f);
	    print_error(ERR_INVALID_EXT,
			"invalid or missing version information.\n");
	}
    }

    print_done();
    chdir("..");
    fclose(f);
}


/* 
   decompresses extension file and returns path to the temporary directory where the
   extension has been stored. 
 */
void unpack_extension(char *package)
{
    int error;
    int fd;
    char tmp[MAXSTR];
    int ftype;

    fprintf(stdout, "Uncompressing files...");

    /* TODO: Do not hardcode temp paths */
    strcpy(TMPDIR, "/tmp/grass.extension.XXXXXX");	/* tmpdir is a global variable */
    mkstemp(TMPDIR);
    fd = open(TMPDIR, O_CREAT, 0777);
    if (fd == -1) {
	print_error(ERR_UNPACK_EXT,
		    "could not create temp directory name: %s",
		    strerror(errno));
	exit(ERR_UNPACK_EXT);
    }

    if (VERBOSE) {
	fprintf(stdout, "\nUncompressing to: %s.\n", TMPDIR);
    }

    /* remove tmp file and create a dir with the same name */
    close(fd);
    remove(TMPDIR);
    mkdir_s(TMPDIR, "0700");

    atexit(&exit_tmp);		/* now need to register an at exit func to remove tmpdir automatically! */

    /* uncompress extension to TMPDIR */
    /* 1. copy file */
    sprintf(tmp, "cp %s %s", package, TMPDIR);
    error = system(tmp);
    if (error < 0) {
	print_error(ERR_UNPACK_EXT,
		    "could not copy extension files to temp dir.\n");
	exit(ERR_UNPACK_EXT);
    }

    /* see what type of archive it is */
    ftype = check_filetype(package);
    if (ftype == TYPE_UNKNOWN) {
	print_warning
	    ("file name not '.tar.gz', '.tgz', '.tar.bz2', '.tbz' or '.zip'. Assuming '.tgz'.\n");
	ftype = TAR_GZIP;
    }
    if (ftype == TAR_GZIP) {
	if (VERBOSE) {
	    sprintf(tmp, "tar -xzvf %s/%s -C %s", TMPDIR, basename(package),
		    TMPDIR);
	}
	else {
	    sprintf(tmp, "tar -xzf %s/%s -C %s", TMPDIR, basename(package),
		    TMPDIR);
	}
    }
    if (ftype == TAR_BZIP2) {
	if (VERBOSE) {
	    sprintf(tmp, "tar -xjvf %s/%s -C %s", TMPDIR, basename(package),
		    TMPDIR);
	}
	else {
	    sprintf(tmp, "tar -xjvf %s/%s -C %s", TMPDIR, basename(package),
		    TMPDIR);
	}
    }
    if (ftype == ZIP) {
	if (VERBOSE) {
	    sprintf(tmp, "unzip %s/%s -d %s", TMPDIR, basename(package),
		    TMPDIR);
	}
	else {
	    sprintf(tmp, "unzip -qq %s/%s -d %s", TMPDIR, basename(package),
		    TMPDIR);
	}
    }
    if (ftype == TAR) {
	if (VERBOSE) {
	    sprintf(tmp, "tar -xvf %s/%s -C %s", TMPDIR, basename(package),
		    TMPDIR);
	}
	else {
	    sprintf(tmp, "tar -xf %s/%s -C %s", TMPDIR, basename(package),
		    TMPDIR);
	}
    }

    error = system(tmp);
    if (error < 0) {
	if (ftype == TAR_GZIP) {
	    print_error(ERR_UNPACK_EXT,
			"could not extract files using 'tar' and 'gzip'. \n \
					Extract manually using 'tar -xzvf %s'.\n", package);
	}
	if (ftype == TAR_BZIP2) {
	    print_error(ERR_UNPACK_EXT, "could not extract files using 'tar' and 'bunzip2'.\n \
				Extract manually using 'tar -xjvf %s'.\n",
			package);
	}
	if (ftype == ZIP) {
	    print_error(ERR_UNPACK_EXT, "could not extract files using 'unzip'.\n \
				Extract manually using 'unzip %s'.\n",
			package);
	}
	exit(ERR_UNPACK_EXT);
    }

    print_done();
}


void query_extension(char *package, char *name, int major, int minor,
		     int revision, char *short_name, char *invocation,
		     char *org_name)
{
    int error;
    char tmp[MAXSTR];

    sprintf(tmp, "%s", basename(package));
    error = chdir(tmp);
    if (error < 0) {
	print_error(ERR_NO_ACCESS_EXT, "extension '%s' not accessible: %s\n",
		    package, strerror(errno));
    }

    fprintf(stdout, "\nExtension '%s', version %i.%i.%i\n\n", name, major,
	    minor, revision);
    dump_ascii("description", "Description");
    dump_ascii("commands", "Commands provided");
    dump_ascii("libs", "Libraries provided");
    dump_ascii("headers", "Header files provided");
    dump_ascii("depends", "Dependencies");
    dump_ascii("bugs", "Bugs");
    sprintf(tmp, "../%s", package);
    list_binaries(tmp);
    dump_ascii("authors", "Author(s)");

    fprintf(stdout, "Type '%s -d %s' to see more detailed information.\n",
	    invocation, org_name);
    fprintf(stdout, "Type '%s -l %s' to see copyright information.\n",
	    invocation, org_name);

    /* run post action script */
    system("sh post");

    exit(0);

}

/*
   Dump contents of config.msg to screen
 */
void print_cfg(void)
{
    FILE *fp;
    char line[MAXSTR];

    fp = fopen("config.msg", "r");
    if (fp != NULL) {
	fprintf(stdout, "\nResult of configuration: \n");
	while (fgets(line, MAXSTR, fp) != NULL) {
	    fprintf(stdout, "%s", line);
	}
	fprintf(stdout, "\n");
    }

    /* remove file afterward */
    remove("config.msg");
}


/*
 */
void source_install(char *package, char *gisbase, char *pkg_short_name,
		    int pkg_major, int pkg_minor, int pkg_revision,
		    char *grass_version)
{
    char tmp[MAXSTR];
    char dir[MAXSTR];
    char install_cmd[MAXSTR];
    char post_cmd[MAXSTR];
    char sysstr[MAXSTR];
    int error;
    struct stat buf;
    FILE *f;

    char *verstr;
    char *grass_major;
    char *grass_minor;
    char *grass_revision;
    int major, minor, revision;

    /* check for valid install directory */
    error = stat(gisbase, &buf);
    if (error < 0) {
	print_error(ERR_INSTALL_EXT, "installation directory invalid: %s\n",
		    strerror(errno));
    }

    /* export environment variables for GRASS 6 build system */
    /* target dir for installation */
    sprintf(GINSTALL_DST, "GINSTALL_DST=%s", gisbase);
    putenv(GINSTALL_DST);
    /*external include path */
    sprintf(tmp, "%s/include", gisbase);
    sprintf(GINSTALL_INC, "GINSTALL_INC=%s", tmp);
    putenv(GINSTALL_INC);
    /* external linker path */
    sprintf(tmp, "%s/lib", gisbase);
    sprintf(GINSTALL_LIB, "GINSTALL_LIB=%s", tmp);
    putenv(GINSTALL_LIB);
    /* path to install files */
    sprintf(GEM_GRASS_DIR, "GEM_GRASS_DIR=%s", gisbase);
    putenv(GEM_GRASS_DIR);

    /* extract GRASS major and minor version numbers */
    verstr = strdup(grass_version);
    grass_major = strtok(verstr, ".");
    grass_minor = strtok(NULL, ".");
    grass_revision = strtok(NULL, ".");
    major = strtol(grass_major, NULL, 10);
    minor = strtol(grass_minor, NULL, 10);
    revision = strtol(grass_revision, NULL, 10);
    free(verstr);


    /* now need to register an exit function that unsets these env vars on termination! */
    atexit(&exit_tmp);

    /* step into target directory and execute configure script */
    sprintf(dir, "%s/src", basename(package));
    error = chdir(dir);
    if (error < 0) {
	print_error(ERR_NO_ACCESS_EXT,
		    "extension files in '%s' not accessible: %s\n", package,
		    strerror(errno));
    }

    if (!SKIP_CFG) {
	if (VERBOSE) {
	    fprintf(stdout, "Running configure script:\n");
	    sprintf(sysstr, "sh %s %s", CONFIG_CMD, CONFIG_OPTS);
	    error = system(sysstr);
	}
	else {
	    fprintf(stdout, "Configuring...");
	    sprintf(sysstr, "sh %s %s --quiet &> %s", CONFIG_CMD, CONFIG_OPTS,
		    TMP_NULL);
	    error = system(sysstr);
	}
	if (error == -1) {
	    print_error(ERR_MISSING_CFG, "could not run configure script.\n");
	}
	if (error > 0) {
	    print_error(ERR_CONFIGURE_EXT, "system configuration failed.\n");
	}
	print_done();
	print_cfg();
    }

    /* export environment variables for generation of HTML documentation directories */
    /* by include/Make/Rules.make */
    sprintf(GEM_EXT_NAME, "GEM_EXT_NAME=%s", pkg_short_name);
    putenv(GEM_EXT_NAME);
    sprintf(tmp, "%i.%i.%i", pkg_major, pkg_minor, pkg_revision);
    sprintf(GEM_EXT_VERSION, "GEM_EXT_VERSION=%s", tmp);
    putenv(GEM_EXT_VERSION);
    /* dump extension info text into two plain ASCII files for inclusion in HTML man page */
    dump_html("../description", TMP_DESCR);
    dump_html("../info", TMP_INFO);
    dump_html("../depends", TMP_DEPS);
    dump_html("../bugs", TMP_BUGS);
    dump_html("../authors", TMP_AUTHORS);
    sprintf(GEM_EXT_DESCR, "GEM_EXT_DESCR=%s", TMP_DESCR);
    putenv(GEM_EXT_DESCR);
    sprintf(GEM_EXT_INFO, "GEM_EXT_INFO=%s", TMP_INFO);
    putenv(GEM_EXT_INFO);
    sprintf(GEM_EXT_DEPS, "GEM_EXT_DEPS=%s", TMP_DEPS);
    putenv(GEM_EXT_DEPS);
    sprintf(GEM_EXT_BUGS, "GEM_EXT_BUGS=%s", TMP_BUGS);
    putenv(GEM_EXT_BUGS);
    sprintf(GEM_EXT_AUTHORS, "GEM_EXT_AUTHORS=%s", TMP_AUTHORS);
    putenv(GEM_EXT_AUTHORS);

    /* now need to register an exit function that unsets these env vars on termination! */
    atexit(&exit_tmp);

    /* check dependencies before compiling everything ... */
    check_dependencies(package, gisbase, grass_version);

    /* now execute Makefile in top-level directory */
    if (VERBOSE) {
	fprintf(stdout, "Running '%s':\n", MAKE_CMD);
	sprintf(sysstr, "%s -f Makefile", MAKE_CMD);
	error = system(sysstr);
    }
    else {
	fprintf(stdout, "Compiling...");
	sprintf(sysstr, "%s -f Makefile &> %s", MAKE_CMD, TMP_NULL);
	error = system(sysstr);
    }
    if (error == -1) {
	if (!VERBOSE) {
	    print_error(ERR_MISSING_CMD,
			"could not run '%s' do you have make tools installed?\n",
			MAKE_CMD);
	}
    }
    if (error > 0) {
	print_error(ERR_COMPILE_EXT, "source code could not be compiled.\n \
			Run again with option -v to see what is causing trouble.\n");
    }
    print_done();

    fprintf(stdout, "Installing...");

    /* check if an uninstall script exists */
    f = fopen("../uninstall", "r");
    if (f == NULL) {
	print_warning("error checking for uninstall script: %s\n \
				Uninstalling this extension may leave orphaned files on your system", strerror(errno));
    }
    else {
	/* it does: create a shell command to copy it upon installation */
	if (VERBOSE) {
	    sprintf(tmp, "cp -vf ../uninstall %s/etc/uninstall.%s ;", gisbase,
		    pkg_short_name);
	    strcpy(UNINSTALL_CMD, tmp);
	}
	else {
	    sprintf(tmp, "cp -f ../uninstall %s/etc/uninstall.%s &> %s ;",
		    gisbase, pkg_short_name, TMP_NULL);
	    strcpy(UNINSTALL_CMD, tmp);
	}
	fclose(f);
    }

    register_extension(gisbase, "src", pkg_short_name, pkg_major, pkg_minor,
		       pkg_revision);

    check_dependencies(package, gisbase, grass_version);

    /* starting with GRASS 6.1.cvs, d.m uses the same menu system as gis.m */
    if ((major == 6) && (minor < 1)) {
	register_entries_gisman(pkg_short_name, gisbase);
    }

    register_entries_gisman2(pkg_short_name, gisbase);

    register_html(pkg_short_name, gisbase, pkg_major, pkg_minor,
		  pkg_revision);

    /* create a shell command for the make install process and installation of extension.db */
    if (VERBOSE) {
	fprintf(stdout, "Running '%s install':\n", MAKE_CMD);
	sprintf(install_cmd, "%s -f Makefile install ; \
					cp -vf %s %s/etc/extensions.db ; chmod -v a+r %s/etc/extensions.db ;", MAKE_CMD, TMPDB, gisbase, gisbase);
    }
    else {
	sprintf(install_cmd, "%s -f Makefile -s install &> %s ; \
					cp -f %s %s/etc/extensions.db &> %s ; chmod a+r %s/etc/extensions.db &> %s ;", MAKE_CMD, TMP_NULL, TMPDB, gisbase, TMP_NULL, gisbase, TMP_NULL);
    }


    /* command to run post action script */
    if (VERBOSE) {
	sprintf(post_cmd, "sh ../post");
    }
    else {
	sprintf(post_cmd, "sh ../post &> %s", TMP_NULL);
    }

    /* make install */
    sprintf(tmp, "%s %s %s %s %s %s", install_cmd, UNINSTALL_CMD, GISMAN_CMD,
	    GISMAN2_CMD, HTML_CMD, post_cmd);

    su(gisbase, tmp);

    print_done();
}


/*
   Install everything from a directory with pre-compiled binaries.
 */
void bin_install(char *package, char *gisbase, char *bins,
		 char *pkg_short_name, int pkg_major, int pkg_minor,
		 int pkg_revision, char *grass_version)
{
    char tmp[MAXSTR];
    char dir[MAXSTR];
    char install_cmd[MAXSTR];
    char post_cmd[MAXSTR];
    int error;
    struct stat buf;
    FILE *f;

    char *verstr;
    char *grass_major;
    char *grass_minor;
    char *grass_revision;
    int major, minor, revision;

    /* check for valid install directory */
    error = stat(gisbase, &buf);
    if (error < 0) {
	print_error(ERR_INSTALL_EXT, "installation directory invalid: %s\n",
		    strerror(errno));
    }

    /* export environment variables for GRASS 6 build system */
    /* target dir for installation */
    sprintf(GINSTALL_DST, "GINSTALL_DST=%s", gisbase);
    putenv(GINSTALL_DST);
    /*external include path */
    sprintf(tmp, "%s/include", gisbase);
    sprintf(GINSTALL_INC, "GINSTALL_INC=%s", tmp);
    putenv(GINSTALL_INC);
    /* external linker path */
    sprintf(tmp, "%s/lib", gisbase);
    sprintf(GINSTALL_LIB, "GINSTALL_LIB=%s", tmp);
    putenv(GINSTALL_LIB);
    /* path to install files */
    sprintf(GEM_GRASS_DIR, "GEM_GRASS_DIR=%s", gisbase);
    putenv(GEM_GRASS_DIR);

    /* extract GRASS major and minor version numbers */
    verstr = strdup(grass_version);
    grass_major = strtok(verstr, ".");
    grass_minor = strtok(NULL, ".");
    grass_revision = strtok(NULL, ".");
    major = strtol(grass_major, NULL, 10);
    minor = strtol(grass_minor, NULL, 10);
    revision = strtol(grass_revision, NULL, 10);
    free(verstr);


    /* now need to register an exit function that unsets these env vars on termination! */
    atexit(&exit_tmp);

    /* step into target directory and execute configure script */
    sprintf(dir, "%s/%s", basename(package), bins);
    error = chdir(dir);
    if (error < 0) {
	print_error(ERR_NO_ACCESS_EXT,
		    "extension file binaries in '%s' not accessible: %s\n",
		    package, strerror(errno));
    }

    /* export environment variables for generation of HTML documentation directories */
    /* by include/Make/Rules.make */
    sprintf(GEM_EXT_NAME, "GEM_EXT_NAME=%s", pkg_short_name);
    putenv(GEM_EXT_NAME);
    sprintf(tmp, "%i.%i.%i", pkg_major, pkg_minor, pkg_revision);
    sprintf(GEM_EXT_VERSION, "GEM_EXT_VERSION=%s", tmp);
    putenv(GEM_EXT_VERSION);
    /* dump extension info text into two plain ASCII files for inclusion in HTML man page */
    dump_html("../description", TMP_DESCR);
    dump_html("../info", TMP_INFO);
    dump_html("../depends", TMP_DEPS);
    dump_html("../bugs", TMP_BUGS);
    dump_html("../authors", TMP_AUTHORS);
    sprintf(GEM_EXT_DESCR, "GEM_EXT_DESCR=%s", TMP_DESCR);
    putenv(GEM_EXT_DESCR);
    sprintf(GEM_EXT_INFO, "GEM_EXT_INFO=%s", TMP_INFO);
    putenv(GEM_EXT_INFO);
    sprintf(GEM_EXT_DEPS, "GEM_EXT_DEPS=%s", TMP_DEPS);
    putenv(GEM_EXT_DEPS);
    sprintf(GEM_EXT_BUGS, "GEM_EXT_BUGS=%s", TMP_BUGS);
    putenv(GEM_EXT_BUGS);
    sprintf(GEM_EXT_AUTHORS, "GEM_EXT_AUTHORS=%s", TMP_AUTHORS);
    putenv(GEM_EXT_AUTHORS);

    /* now need to register an exit function that unsets these env vars on termination! */
    atexit(&exit_tmp);

    /* check dependencies before installing ... */
    check_dependencies(package, gisbase, grass_version);

    fprintf(stdout, "Installing...");

    /* check if an uninstall script exists */
    f = fopen("../uninstall", "r");
    if (f == NULL) {
	print_warning("error checking for uninstall script: %s\n \
				Uninstalling this extension may leave orphaned files on your system", strerror(errno));
    }
    else {
	/* it does: create a shell command to copy it upon installation */
	if (VERBOSE) {
	    sprintf(tmp, "cp -vf ../uninstall %s/etc/uninstall.%s ;", gisbase,
		    pkg_short_name);
	    strcpy(UNINSTALL_CMD, tmp);
	}
	else {
	    sprintf(tmp, "cp -f ../uninstall %s/etc/uninstall.%s &> %s ;",
		    gisbase, pkg_short_name, TMP_NULL);
	    strcpy(UNINSTALL_CMD, tmp);
	}
	fclose(f);
    }

    register_extension(gisbase, bins, pkg_short_name, pkg_major, pkg_minor,
		       pkg_revision);
    check_dependencies(package, gisbase, grass_version);

    /* starting with GRASS 6.1.cvs, d.m uses the same menu system as gis.m */
    if ((major == 6) && (minor < 1)) {
	register_entries_gisman(pkg_short_name, gisbase);
    }

    register_entries_gisman2(pkg_short_name, gisbase);

    register_html(pkg_short_name, gisbase, pkg_major, pkg_minor,
		  pkg_revision);

    /* create a shell command for the make install process and installation of extension.db */
    /* we will use a local copy of the make command for this (extension author has to supply it). */
    if (VERBOSE) {
	fprintf(stdout, "Running '%s install':\n", MAKE_CMD);
	sprintf(install_cmd, "bin/%s -f Makefile install ; \
					cp -vf %s %s/etc/extensions.db ; chmod -v a+r %s/etc/extensions.db ;", MAKE_CMD, TMPDB, gisbase, gisbase);
    }
    else {
	sprintf(install_cmd, "bin/%s -f Makefile -s install &> %s ; \
					cp -f %s %s/etc/extensions.db &> %s ; chmod a+r %s/etc/extensions.db &> %s ;", MAKE_CMD, TMP_NULL, TMPDB, gisbase, TMP_NULL, gisbase, TMP_NULL);
    }

    /* command to run post action script */
    if (VERBOSE) {
	sprintf(post_cmd, "sh ../post");
    }
    else {
	sprintf(post_cmd, "sh ../post &> %s", TMP_NULL);
    }

    /* make install */
    sprintf(tmp, "%s %s %s %s %s %s", install_cmd, UNINSTALL_CMD, GISMAN_CMD,
	    GISMAN2_CMD, HTML_CMD, post_cmd);

    su(gisbase, tmp);

    print_done();
}


void test_install(char *package, char *gisbase, char *pkg_short_name,
		  int pkg_major, int pkg_minor, int pkg_revision,
		  char *grass_version)
{

    char tmp[MAXSTR];
    char dir[MAXSTR];
    char sysstr[MAXSTR];
    int error;
    struct stat buf;
    FILE *f;

    char *verstr;
    char *grass_major;
    char *grass_minor;
    char *grass_revision;
    int major, minor, revision;

    /* check for valid install directory */
    error = stat(gisbase, &buf);
    if (error < 0) {
	print_error(ERR_INSTALL_EXT, "installation directory invalid: %s\n",
		    strerror(errno));
    }

    /* export environment variables for GRASS 6 build system */
    /* target dir for installation */
    sprintf(GINSTALL_DST, "GINSTALL_DST=%s", gisbase);
    putenv(GINSTALL_DST);
    /*external include path */
    sprintf(tmp, "%s/include", gisbase);
    sprintf(GINSTALL_INC, "GINSTALL_INC=%s", tmp);
    putenv(GINSTALL_INC);
    /* external linker path */
    sprintf(tmp, "%s/lib", gisbase);
    sprintf(GINSTALL_LIB, "GINSTALL_LIB=%s", tmp);
    putenv(GINSTALL_LIB);
    /* path to install files */
    sprintf(GEM_GRASS_DIR, "GEM_GRASS_DIR=%s", gisbase);
    putenv(GEM_GRASS_DIR);

    /* extract GRASS major and minor version numbers */
    verstr = strdup(grass_version);
    grass_major = strtok(verstr, ".");
    grass_minor = strtok(NULL, ".");
    grass_revision = strtok(NULL, ".");
    major = strtol(grass_major, NULL, 10);
    minor = strtol(grass_minor, NULL, 10);
    revision = strtol(grass_revision, NULL, 10);
    free(verstr);

    /* now need to register an exit function that unsets these env vars on termination! */
    atexit(&exit_tmp);

    /* step into target directory and execute configure script */
    sprintf(dir, "%s/src", basename(package));
    error = chdir(dir);
    if (error < 0) {
	print_error(ERR_NO_ACCESS_EXT,
		    "extension files in '%s' not accessible: %s\n", package,
		    strerror(errno));
    }

    if (!SKIP_CFG) {
	if (VERBOSE) {
	    fprintf(stdout, "Running configure script:\n");
	    sprintf(sysstr, "sh %s %s", CONFIG_CMD, CONFIG_OPTS);
	    error = system(sysstr);
	}
	else {
	    fprintf(stdout, "Configuring...");
	    sprintf(sysstr, "sh %s %s --quiet &> %s", CONFIG_CMD, CONFIG_OPTS,
		    TMP_NULL);
	    error = system(sysstr);
	}
	if (error == -1) {
	    print_error(ERR_MISSING_CFG, "could not run configure script.\n");
	}
	if (error > 0) {
	    print_error(ERR_CONFIGURE_EXT, "system configuration failed.\n");
	}
	print_done();
	print_cfg();
    }

    /* export environment variables for generation of HTML documentation directories */
    /* by include/Make/Rules.make */
    sprintf(GEM_EXT_NAME, "GEM_EXT_NAME=%s", pkg_short_name);
    putenv(GEM_EXT_NAME);
    sprintf(tmp, "%i.%i.%i", pkg_major, pkg_minor, pkg_revision);
    sprintf(GEM_EXT_VERSION, "GEM_EXT_VERSION=%s", tmp);
    putenv(GEM_EXT_VERSION);
    /* dump extension info text into two plain ASCII files for inclusion in HTML man page */
    dump_plain("../description", TMP_DESCR);
    dump_plain("../info", TMP_INFO);
    dump_plain("../depends", TMP_DEPS);
    dump_plain("../bugs", TMP_BUGS);
    dump_plain("../authors", TMP_AUTHORS);
    sprintf(GEM_EXT_DESCR, "GEM_EXT_DESCR=%s", TMP_DESCR);
    putenv(GEM_EXT_DESCR);
    sprintf(GEM_EXT_INFO, "GEM_EXT_INFO=%s", TMP_INFO);
    putenv(GEM_EXT_INFO);
    sprintf(GEM_EXT_DEPS, "GEM_EXT_DEPS=%s", TMP_DEPS);
    putenv(GEM_EXT_DEPS);
    sprintf(GEM_EXT_BUGS, "GEM_EXT_BUGS=%s", TMP_BUGS);
    putenv(GEM_EXT_BUGS);
    sprintf(GEM_EXT_AUTHORS, "GEM_EXT_AUTHORS=%s", TMP_AUTHORS);
    putenv(GEM_EXT_AUTHORS);

    /* now need to register an exit function that unsets these env vars on termination! */
    atexit(&exit_tmp);

    /* check dependencies before compiling everything ... */
    check_dependencies(package, gisbase, grass_version);

    /* now execute Makefile in top-level directory */
    if (VERBOSE) {
	fprintf(stdout, "Running '%s':\n", MAKE_CMD);
	sprintf(sysstr, "%s -f Makefile", MAKE_CMD);
	error = system(sysstr);

    }
    else {
	fprintf(stdout, "Compiling...");
	sprintf(sysstr, "%s -f Makefile &> %s", MAKE_CMD, TMP_NULL);
	error = system(sysstr);
    }
    if (error == -1) {
	if (!VERBOSE) {
	    print_error(ERR_MISSING_CMD,
			"could not run '%s' do you have make tools installed?\n",
			MAKE_CMD);
	}
    }
    if (error > 0) {
	print_error(ERR_COMPILE_EXT, "source code could not be compiled.\n \
			Run again with option -v to see what is causing trouble.\n");
    }
    print_done();

    fprintf(stdout, "Installing...");

    /* check if an uninstall script exists */
    f = fopen("../uninstall", "r");
    if (f == NULL) {
	print_warning("error checking for uninstall script: %s\n \
				Uninstalling this extension may leave orphaned files on your system", strerror(errno));
    }
    else {
	fclose(f);
    }

    register_extension(gisbase, "src", pkg_short_name, pkg_major, pkg_minor,
		       pkg_revision);
    check_dependencies(package, gisbase, grass_version);

    /* starting with GRASS 6.1.cvs, d.m uses the same menu system as gis.m */
    if ((major == 6) && (minor < 1)) {
	register_entries_gisman(pkg_short_name, gisbase);
    }

    register_entries_gisman2(pkg_short_name, gisbase);

    register_html(pkg_short_name, gisbase, pkg_major, pkg_minor,
		  pkg_revision);

    fprintf(stdout, "(skipping '%s install')...", MAKE_CMD);

    print_done();
}


/*
   Run the uninstall script that was (hopefully) provided by the packager.
   Check for unsatisfied dependencies and warn/abort accordingly.
 */
void uninstall(char *package, char *pkg_short_name, char *gisbase,
	       char *grass_version)
{
    char tmp[MAXSTR];
    char script[MAXSTR];
    int error;
    struct stat buf;
    int no_script;

    char *verstr;
    char *grass_major;
    char *grass_minor;
    char *grass_revision;
    int major, minor, revision;

    fprintf(stdout, "Un-installing...");

    /* export environment variables for uninstall script */
    sprintf(UNINSTALL_BASE, "UNINSTALL_BASE=%s", gisbase);
    putenv(UNINSTALL_BASE);

    /* extract GRASS major and minor version numbers */
    verstr = strdup(grass_version);
    grass_major = strtok(verstr, ".");
    grass_minor = strtok(NULL, ".");
    grass_revision = strtok(NULL, ".");
    major = strtol(grass_major, NULL, 10);
    minor = strtol(grass_minor, NULL, 10);
    revision = strtol(grass_revision, NULL, 10);
    free(verstr);

    /* now need to register an exit function that unsets these env vars on termination! */
    atexit(&exit_tmp);

    /* deregister extension  */
    deregister_extension(package, pkg_short_name, gisbase);

    /* deregister menu entries in GIS Manager */
    /* starting with GRASS 6.1.cvs, d.m uses the same menu system as gis.m */
    if ((major == 6) && (minor < 1)) {
	error = deregister_entries_gisman(pkg_short_name, gisbase);
	if (error == -1) {
	    print_warning("GIS Manager menu entries could not be removed.\n");
	    strcpy(GISMAN_CMD, "");
	}
	if (error == 0) {
	    print_warning("no entries found to remove from GIS Manager.\n");
	    strcpy(GISMAN_CMD, "");
	}
    }

    /* deregister menu entries in GIS Manager 2 */
    deregister_entries_gisman2(pkg_short_name, gisbase);

    /* deregister HTML entries and delete man pages */
    deregister_html(pkg_short_name, gisbase);

    /* check if an uninstall script exists */
    sprintf(script, "%s/etc/uninstall.%s", gisbase, pkg_short_name);
    no_script = 0;
    error = stat(script, &buf);
    if (error < 0) {
	print_warning("no uninstall script available for this extension.\n \
			Unneeded files may have been left on your system.\n");
	no_script = 1;
    }
    if (no_script) {
	print_warning("error checking for uninstall script: %s\n \
				Uninstalling this extension may leave orphaned files on your system", strerror(errno));
    }
    else {
	/* it does: create a shell command to execute and then delete it upon de-installation */
	if (VERBOSE) {
	    sprintf(tmp, "sh %s ; rm -vf %s ; \
					 		rm -vrf %s/docs/extensions/%s ; rm -vf %s/etc/dm/gem-entries/%s ; \
							cp -vf %s %s/etc/extensions.db ; chmod -v a+r %s/etc/extensions.db ;", script, script, gisbase, pkg_short_name, gisbase, pkg_short_name, TMPDB, gisbase, gisbase);
	    strcpy(UNINSTALL_CMD, tmp);
	}
	else {
	    sprintf(tmp, "sh %s &> %s ; rm -vf %s &> %s ; \
					 		rm -vrf %s/docs/extensions/%s &> %s ; rm -vf %s/etc/dm/gem-entries/%s &> %s ; \
							cp -vf %s %s/etc/extensions.db &> %s ; chmod -v a+r %s/etc/extensions.db &> %s ;", script, TMP_NULL, script, TMP_NULL, gisbase, pkg_short_name, TMP_NULL, gisbase, pkg_short_name, TMP_NULL, TMPDB, gisbase, TMP_NULL, gisbase, TMP_NULL);
	    strcpy(UNINSTALL_CMD, tmp);
	}
    }

    /* do the uninstall! */
    sprintf(tmp, "%s %s %s", UNINSTALL_CMD, GISMAN_CMD, HTML_CMD);

    su(gisbase, tmp);

    print_done();
}


int source_clean(char *package)
{
    char dir[MAXSTR];
    char sysstr[MAXSTR];
    int error;

    /* step into target directory */
    sprintf(dir, "%s/src", basename(package));
    error = chdir(dir);
    if (error < 0) {
	print_error(ERR_NO_ACCESS_EXT, "extension '%s' not accessible: ",
		    package);
    }

    /* now execute Makefile and 'clean' from top-level directory */
    if (VERBOSE) {
	fprintf(stdout, "Running '%s clean':\n", MAKE_CMD);
	sprintf(sysstr, "%s -f Makefile clean", MAKE_CMD);
	error = system(sysstr);
    }
    else {
	fprintf(stdout, "Cleaning up...");
	sprintf(sysstr, "%s -f Makefile -s clean &> %s", MAKE_CMD, TMP_NULL);
	error = system(sysstr);
    }

    if (error == -1) {
	print_error(ERR_MISSING_CMD,
		    "could not run '%s clean' do you have make tools installed?\n",
		    MAKE_CMD);
    }
    else {
	print_done();
    }

    /* run post action script */
    system("sh ../post");

    return (0);
}


/* 
   Restores HTML links and GIS Manager menus, e.g. after an update of GRASS
 */
void restore(char *gisbase, char *grass_version)
{
    int num_restored;
    char tmp[MAXSTR];

    char *verstr;
    char *grass_major;
    char *grass_minor;
    char *grass_revision;
    int major, minor, revision;


    /* extract GRASS major and minor version numbers */
    verstr = strdup(grass_version);
    grass_major = strtok(verstr, ".");
    grass_minor = strtok(NULL, ".");
    grass_revision = strtok(NULL, ".");
    major = strtol(grass_major, NULL, 10);
    minor = strtol(grass_minor, NULL, 10);
    revision = strtol(grass_revision, NULL, 10);
    free(verstr);

    fprintf(stdout, "Restoring...");

    /* starting with GRASS 6.1.cvs, d.m uses the same menu system as gis.m */
    if ((major == 6) && (minor < 1)) {
	num_restored = restore_entries_gisman(gisbase);
	if (VERBOSE) {
	    fprintf(stdout, "\nRestored entries for GIS Manager: %i\n",
		    num_restored);
	}
    }

    num_restored = restore_html(gisbase);
    if (VERBOSE) {
	fprintf(stdout, "\nRestored links in index.hml: %i\n", num_restored);
    }

    if (num_restored > 0) {
	/* copy restored files to GRASS install tree */
	if (VERBOSE) {
	    sprintf(tmp,
		    "cp -f %s %s/etc/dm/menu.tcl ; chmod a+r %s/etc/dm/menu.tcl ; \
							cp -f %s %s/docs/html/index.html ; chmod a+r %s/docs/html/index.html",
		    TMP_GISMAN, gisbase, gisbase, TMP_HTML, gisbase, gisbase);
	}
	else {
	    sprintf(tmp,
		    "cp -f %s %s/etc/dm/menu.tcl ; chmod a+r %s/etc/dm/menu.tcl &> %s ; \
							cp -f %s %s/docs/html/index.html ; chmod a+r %s/docs/html/index.html",
		    TMP_GISMAN, gisbase, gisbase, TMP_NULL, TMP_HTML, gisbase,
		    gisbase);
	}

	su(gisbase, tmp);

    }

    if (num_restored == 0) {
	print_error(ERR_RESTORE, "could not find anything to restore.\n");
    }
    else {
	print_done();
    }
}


/*
   List all extensions installed on system with versions and dependencies
 */
void list_extensions(char *gisbase)
{
    char file[MAXSTR];
    FILE *f_in;

    fprintf(stdout, "\nExtensions in '%s' (name, version, type, depends):\n",
	    gisbase);

    /* check, if extensions.db exists and is readable */
    sprintf(file, "%s/etc/extensions.db", gisbase);
    f_in = fopen(file, "r");
    if (f_in == NULL) {
	if (errno == ENOENT) {
	    /* file does not yet exist */
	    fprintf(stderr, "NONE.\n");
	    fclose(f_in);
	    exit(0);
	}
	else {
	    /* sth. strange happened */
	    fclose(f_in);
	    print_error(ERR_LIST, "checking for file '%s': %s\n", file,
			strerror(errno));
	}
    }
    fclose(f_in);

    dump_ascii(file, "");
}


/*
   A bit of a misnomer: this does not actually run the post
   script but it exports all necessary env vars.
 */
void run_post(char *package, int action, char *bins, char *gisbase)
{
    char tmp[MAXSTR];
    char tmp2[MAXSTR];

    switch (action) {
    case INSTALL:
	sprintf(GEM_ACTION, "GEM_ACTION=INSTALL");
	break;
    case BIN_INSTALL:
	sprintf(GEM_ACTION, "GEM_ACTION=INSTALL");
	break;
    case QUERY:
	sprintf(GEM_ACTION, "GEM_ACTION=QUERY");
	break;
    case CLEAN:
	sprintf(GEM_ACTION, "GEM_ACTION=CLEAN");
	break;
    case LICENSE:
	sprintf(GEM_ACTION, "GEM_ACTION=LICENSE");
	break;
    case DETAILS:
	sprintf(GEM_ACTION, "GEM_ACTION=DETAILS");
	break;
    default:
	break;
    }
    putenv(GEM_ACTION);

    if (gisbase != NULL) {
	sprintf(INSTALL_BASE, "INSTALL_BASE=%s", gisbase);
    }
    else {
	sprintf(INSTALL_BASE, "INSTALL_BASE=UNDEFINED");
    }
    putenv(INSTALL_BASE);

    /* export absolute path to current working dir */
    getcwd(tmp, MAXSTR);
    sprintf(tmp2, "%s/%s/src", tmp, basename(package));
    sprintf(EXT_BASE, "EXT_BASE=%s", tmp2);
    putenv(EXT_BASE);

    if (bins == NULL) {
	sprintf(INSTALL_TYPE, "INSTALL_TYPE=src");
    }
    else {
	sprintf(INSTALL_TYPE, "INSTALL_TYPE=%s", bins);
    }
    putenv(INSTALL_TYPE);

    sprintf(tmp, "%i", FORCE);
    sprintf(GEM_FORCE, "GEM_FORCE=%s", tmp);
    putenv(GEM_FORCE);

    sprintf(tmp, "%i", VERBOSE);
    sprintf(GEM_VERBOSE, "GEM_VERBOSE=%s", tmp);
    putenv(GEM_VERBOSE);

    sprintf(GEM_GUI, "GEM_GUI=0");
    putenv(GEM_GUI);


    /* now need to register an exit function that unsets these env vars on termination! */
    atexit(&exit_tmp);
}
