/***************************************************************************
 *            main.c
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


/* TODO:
				
	for 1.04:			
	
	EASY STUFF:
	
	
	- tried to install GeneralStatistics w/o first installing RasterTools or anything else: attempts to install, fails because
	  of missing raster tools headers! Even though line 379 in reg_deps.c should check for this case !!!
	
	- include $GISBASE/lib in linker path for compilation of extensions [NOT FIXED: see lines preceeding INSTALL action case: 693
										-L is added to command line, but problems still exist]
		MAYBE EXPORT LD_LIBRARY_PATH ??? (only add $GISBASE/lib if it does not already exist)
		
	- version number in HTML documentation index seems to not get updated when installing a newer version of an extension
	
	- remember to update version number in globals.h
		
		
	for 1.2 (GRASS 6.4 ?):
	- make GRASS store its ./configure command line options in a file in the GISBASE/etc directory, so that
	  it will be possible for GEM to automatically configure extensions according to the system setup
	  [this means that configure file needs to be kept in sync with GEM; --configure option can be used
	   to overwrite this behaviour]
	- configure script should not fail but disable options and create config.msgs the
	  contents of this should be displayed and deleted afterwards
	- check if it works with this Mac version of GRASS, as well: http://openosx.com/grass/
	- before release: any problems expected with native Win32-Kompilation?:
		- location for tmp-files?
	- a simple wrapper g.install with GRASS style parameters. Should start an xterm and ask for su pwd,
	  if necessary
	- action to check dependencies of installed extensions
	- instead of aborting on each failed dependency: build list of all failed
	  dependencies before aborting
	- mechanism to recursively download and install missing deps from a
	  provided list of URLs
	- instead of calling external tar, switch to tarlib and zlib
	- action --validate for checking an extension archive/dir
	- add link to HTML help index into each extension's submenu in GIS Manager
	- g.install with a real custom-made GUI
	[VARIABLE GUI must be set appropriately in run_post() for post script]	 
	  this should be an independent Tcl/Tk GUI: install/uninstall/query extensions
	- extension database on the internet that can be queried and used with either gem
	  or g.install
	- entries for QGIS
	- simplified scheme for installing just a single module
	- new scheme for registering menu entries in GIS Manager with stable hooks in menu.tcl
		  
			  					  
  NEED HELP
    - GRASS' make install installs all files BENEATH top level dir with UID
	  set to benni !!! Is this intentional?
	- provide gem61 as link in /usr/local/bin as part of grass 6.1 base install
	- description.html should not contain </body> or </html> as Rules.Make seems
	  to append those ?
	- source install copies files COPYING README REQUIREMENTS.html to somewhere (but where?)
		-> into <extension>/src but where else ?	
	- menu.tcl in GRASS 6.0.0 has "all options 1", but 6.1 will have "all options $tmenu".
	  How to cater for such things? Maybe in the future there should always be an
	  Xtns menu provided by GIS Manager and always delimited by reliable tags
	- d.m window is a bit to small horizontally

  CAVEATS:
  	- does not allow installation of different versions of the same extension within
	  the same GRASS bin tree
	- does not have a real upgrading mechanism: option --upgrade has been disabled for now
	- restore cannot fix partially corrupted entries in HTML and TCL files
	- Links to other extension's modules will only work if those extensions are installed
	
  DOCS:
  
  	- skeleton contains GPL as default license. Creators of new extensions
	  need to be aware of this!
  
  	- menu.tcl will be backed up as menu.tcl.gem.bak
  
    - HTML docs: users must prefix references to GRASS modules outside the Extension with
	  "../../html/" !!!	
    - description files: things inside "<" and ">" will be filtered out as HTML tags, even
	  if they are none!
	- make clear that there are some files which will be rendered both as text and HTML
	  and therefore need things like <br> while others DO NOT (which ones?)!

    - configure script should not fail but disable options and create 'config.msgs'
  	- deletes config.msg after display

	- State clearly that it is recommended to uninstall an older
	  extension version before installing a new one. STATE CLEARLY that --upgrade is rather
	  another version of --force than a real updating mechanism (RENAME to --newer)
	- document all env vars that gem sets
	- STATE CLEARLY, that users should not meddle with stuff in index.html, menu.tcl,
	  gem-entries/ and docs/extensions/ !!!
	- provide a unix man page, ASCII and HTML documentation (write HTML and convert)
	- files in the skeleton should always be present, even if they do not containing
	  information
	- make it clear, that uninstall and postinstall are run with su privileges! thus,
	  author must make sure, that only objects in the GRASS install tree are affected
	- user may specify either file name or extension name for uninstall action
	- user may list installed extensions by using -q w/o filename. Explain
	  what is meant by type (e.g. 'src')
	- uninstallation works only by extension name, NOT file name !
	- compiling extensions on a GRASS install with all but the most basic options
	  disabled should give very portable binaries! (statically linked binaries?)
	  
  BUGS:
  	- remove cva, install again: WARNING: list item 'cva' exists in index.html (?)
	- superfluous warning upon uninstall of extension w/o "entries-gisman"
	- due to bad command line parsing, listing installed extensions only works like this:
		./gem --grass=/usr/local/grass-6.1.cvs -q
	  NOT like this:
	  	./gem -q --grass=/usr/local/grass-6.1.cvs

  
*/

#include <getopt.h>
#include <fcntl.h>

#define LOCAL
#include "globals.h"


void show_help ( void ) {
	fprintf (stdout, "Usage: gem [OPTION] [ACTION] [FILE|DIR]\n");	
	fprintf (stdout, "Install a GRASS extension from FILE or DIR.\n");
	fprintf (stdout, "Manage (installed) GRASS extension(s).\n");
	fprintf (stdout, "\nPossible ACTIONs are:\n");
	fprintf (stdout, "  -i, --install=EXT\tinstall a GRASS extension\n");
	fprintf (stdout, "  -u, --uninstall=EXT\tremove an extension from GRASS\n");
	fprintf (stdout, "  -q, --query=EXT\tdisplay information about extension/list installed\n");
	fprintf (stdout, "  -d, --details=EXT\tdisplay additional details about an extension\n");		
	fprintf (stdout, "  -c, --clean=EXT\tclean extension's source code directories\n");
	fprintf (stdout, "  -t, --test=EXT\tconfigure and compile extension, but don't install\n");
	fprintf (stdout, "  -l, --license=EXT\tshow copyright information for an extension\n");
	fprintf (stdout, "  -r, --restore\t\trecreate HTML links and GIS Manager entries\n");	
	fprintf (stdout, "  -h, --help\t\tdisplay this help and exit\n");
	fprintf (stdout, "  -V, --version\t\toutput version information and exit\n\n");	
	fprintf (stdout, "\nPossible OPTIONs are:\n");
	fprintf (stdout, "  -g, --grass=PATH\tpath to GRASS installation dir\n");	
	fprintf (stdout, "  -b, --binary=NAME\tno compilation: use binary files for system NAME\n");
	fprintf (stdout, "  -f, --force\t\tforce action, regardless of dependencies\n");
	fprintf (stdout, "  -v, --verbose\t\tdisplay detailed status information\n");
	fprintf (stdout, "  -s, --skip-config\tskip configure script\n");	
	fprintf (stdout, "  -x, --config-opts=OPTS\tpass OPTS to configure script\n");
	fprintf (stdout, "  -o, --options=OPTS\toptions to pass to the C compiler/linker\n");
	fprintf (stdout, "  -C, --config-cmd=CMD\tDefine custom 'configure' command (default=configure)\n");	
	fprintf (stdout, "  -m, --make-cmd=CMD\tDefine custom 'make' command (default=make)\n");	
	fprintf (stdout, "\nWhen run from within a GRASS session, locations of libs, header files\n");
	fprintf (stdout, "and installation target dir will be assumed to match those of the active\n");
	fprintf (stdout, "GRASS version. ");
	fprintf (stdout, "Option -g can be used to override these or install extensions\nfrom outside");
	fprintf (stdout, "of a GRASS session.\n");
	fprintf (stdout, "Per default, extensions will be compiled from source and then installed.\n");
	fprintf (stdout, "If the exension package contains binaries for the user's platform, they can\n");
	fprintf (stdout, "be installed instead using the -b option. ");
	fprintf (stdout, "For installation from source code, a C compiler and make tools are needed.\n");
	fprintf (stdout, "\nExample:\n");
	fprintf (stdout, "\tgem -b macosx --grass=/usr/local/grass-6.0.0 -i myExtension\n");
	fprintf (stdout, "Installs the MacOS X binaries for 'myExtension' in /usr/local/grass-6.0.0.\n");
	exit (0);	
}


void show_details ( char *package ) {
	int error;
	char tmp [MAXSTR];

	sprintf (tmp, "%s", basename (package) );
	error = chdir ( tmp );
	if ( error < 0 ) {
		print_error ( ERR_NO_ACCESS_EXT, "extension '%s' not accessible: (%s)\n", package, strerror (errno));
	}
	
	dump_ascii ("info", "Detailed information");


	/* run post action script */
	system ("sh post");

	exit (0);	
}


void show_license ( char *package ) {
	int error;
	char tmp [MAXSTR];
	
	sprintf (tmp, "%s", basename (package) );
	error = chdir ( tmp );
	if ( error < 0 ) {
		print_error ( ERR_NO_ACCESS_EXT, "extension '%s' not accessible: (%s)\n", package, strerror (errno));
	}

	dump_ascii ("license", "Detailed information");
	
	/* run post action script */
	system ("sh post");

	exit (0);
}


void show_version ( void ) {
	fprintf (stdout, "gem (GRASS extensions manager) %.2f\n", PROGVERSION);
	fprintf (stdout, "Written by Benjamin Ducke\n");
	fprintf (stdout, "\nCopyright (C) 2005 Benjamin Ducke\n");
	fprintf (stdout, "This is free software; see the source for copying conditions.  There is NO\n");
	fprintf (stdout, "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	exit (0);
}


/* determine options to pass to extension's configure script */
/* TODO: check, if system configuration meets a set of requirements */
/* THIS FUNCTION IS CURRENTLY NOT USED */
void get_configure_options ( char *gisbase ) {

	FILE *fp;
	char str [MAXSTR];

	if ( strcmp ( CONFIG_OPTS, "" ) ) {
		/* if user has specified config options on the GEM command line: override anything else */
		return;
	}
	
	/* check if GISBASE/etc/config.system exists and if so, read options from it */
	sprintf ( str, "%s/etc/config.system", gisbase );
	fp = fopen ( str, "r" );
	if ( fp == NULL ) {
		print_warning ("could not open %s for read access. Using default configure options.\n", str );
		return;
	}

	/* config.system may also contain nothing, only comments and/or whitespace */	
	if ( nc_fgets_nb ( str, MAXSTR, fp )  != NULL ) {
		strcpy ( CONFIG_OPTS, str );
	}
}


int main (int argc, char *argv[]) {
	char *gisbase;
	char *grass_version;	
	char *grass_major;
	char *grass_minor;
	char *grass_revision;	
	char *tmp;
	char *url;
	char *filepart;
	char version_file [MAXSTR];
	char package [MAXSTR];
	char orgname [MAXSTR];
	char pkg_name [MAXSTR];
	char *bins;
	int pkg_major, pkg_minor, pkg_revision;
	FILE *f;
	int fd;
	
	char pkg_short_name [MAXSTR];
	char invocation [MAXSTR];
	
	char coptions [MAXSTR];
	
	int major, minor, revision;
	int option;
	int action;
	int valid;
	
	struct stat buf;
	struct stat buf2;
	int error;
	int is_directory = 0;
	DIR *dir;
	struct dirent *dir_entry;
	int dir_found;
	
	int option_index = 0;
	static struct option long_options[] = {
		{ "install", 1, NULL, 'i' },
		{ "uninstall", 1, NULL, 'u' },
		{ "query", 2, NULL, 'q' },
		{ "details", 1, NULL, 'd' },
		{ "clean", 1, NULL, 'c' },
		{ "test", 1, NULL, 't' },
		{ "license", 1, NULL, 'l' },
		{ "restore", 0, NULL, 'r' },
		{ "help", 0, NULL, 'h' },
		{ "version", 0, NULL, 'V' },
		
		{ "grass", 1, NULL, 'g' },
		{ "options", 1, NULL, 'o' },
		{ "binary", 1, NULL, 'b' },
		{ "force", 0, NULL, 'f' },
		{ "verbose", 0, NULL, 'v' },
		{ "skip-config", 0, NULL, 's' },
		{ "config-opts", 1, NULL, 'x' },		

		{ "config-cmd", 1, NULL, 'C' },		
		{ "make-cmd", 1, NULL, 'm' },		
		
		{ 0, 0, 0, 0 }
	};

	
	/* set global variables to defaults */
	VERBOSE = 0;
	TMPCLEAN = 0;
	TMPDBCLEAN = 0;
	FORCE = 0;
	UPGRADE = 0;
	ERROR = 0;
	WARNINGS = 0;
	SKIP_CFG = 0;
	
	strcpy (GISMAN_CMD, "");
	strcpy (GISMAN2_CMD, "");
	strcpy (QGIS_CMD, "");
	strcpy (UNINSTALL_CMD, "");	
	strcpy (HTML_CMD, "");

	strcpy (TMPDIR,"");
	strcpy (TMPDB,"");
	strcpy (TMP_GISMAN, "");
	strcpy (TMP_DESCR,"");
	strcpy (TMP_INFO,"");
	strcpy (TMP_DEPS,"");
	strcpy (TMP_BUGS,"");
	strcpy (TMP_AUTHORS,"");
	strcpy (TMP_HTML,"");
	strcpy (TMP_NULL,"");
	
	strcpy (CONFIG_OPTS,"");
	
	strcpy (CONFIG_CMD,"configure");
	strcpy (MAKE_CMD,"make");
	
	getcwd ( CWD, MAXSTR );	
	
	/* reset terminal colors */
	fprintf (stdout, "\033[0m");
	
	tmp = malloc (sizeof(char) * MAXSTR);
	strcpy (invocation, argv[0]);
	
	/* all output should be unbuffered */
	setvbuf (stdout, (char *) NULL, _IONBF, 0);

	/* if run with no arguments: show help */
	if ( argc == 1 ) {
		/* show usage info and exit */
		show_help ();
		exit (0);
	}
	
	atexit ( &exit_msg ); /* show a message after program termination */		

	valid = 0;
	bins = NULL;
	gisbase = NULL;
	
	opterr = 0;
	option = getopt_long ( argc, argv, ":i:u:q:d:c:C:t:l:m:o:x:rhVg:b:fvs", long_options, &option_index );
	while ( option  != -1 ) {
											
		if ( option == '?' ) {
			print_error (ERR_INVOCATION,"unknown option or action specified.\n");
		}
		
		
		/* check for missing arguments */
		if ( option == ':' ) {
			if ( (optopt == 'i') || (optopt == 'u') || (optopt == 'd') ||
			 	(optopt == 'c') || (optopt == 't') || (optopt == 'l') || (optopt == 'r') ) {
					print_error (ERR_INVOCATION,"missing file or directory name.\n");
				}			
			if ( optopt == 'g' ) {
				print_error (ERR_INVOCATION,"missing path to GRASS 6.\n");
			}
			if ( optopt == 'b' ) {
				print_error (ERR_INVOCATION,"missing name of binary architecture.\n");
			}
			if ( optopt == 'b' ) {
				print_error (ERR_INVOCATION,"missing configure options.\n");
			}			
			if ( optopt == 'q' ) {
				/* '-q' w/o filename is list action */
				action = LIST;
				valid ++;
				break;
			}
		}

		if ( (option == 'i') || (option == 'u') || (option == 'q') || (option == 'd') ||
			 (option == 'c') || (option == 't') || (option == 'l') || (option == 'r') ||
			 (option == 'h') || (option == 'V') ) {
		 	/* got a valid action specifier */
			valid ++;
			/* set action accordingly */
			switch ( option ) {
				case 'i' :
					if ( action != BIN_INSTALL ) {
						action = INSTALL;
					}
				break;
				case 'u' :
					action = UNINSTALL;
				break;
				case 'q' :
					action = QUERY;
				break;
				case 'd' :
					action = DETAILS;
				break;
				case 'c' :
					action = CLEAN;
				break;
				case 't' :
					action = TEST_INSTALL;
				break;
				case 'l' :
					action = LICENSE;
				break;
				case 'r' :
					action = RESTORE;
				break;
				case 'h' :
					action = HELP;
				break;
				case 'V' :
					action = VERSION;
				break;				
			}
			if ( optarg != NULL ) {
				/* save package name as given on command line */
				strcpy ( package, optarg );
				/* orgname will always preserve the commandline option */
				strcpy ( orgname, optarg );
			}			
		}		
		
		/* set options */
		if ( option == 'g' ) {
			gisbase = malloc ( sizeof (char) * ( strlen ( optarg ) + 1 ) );
			strcpy ( gisbase, optarg );			
		}
		if ( option == 'b' ) {
			bins = malloc ( sizeof (char) * ( strlen ( optarg ) + 1 ) );
			strcpy ( bins, optarg );
			action = BIN_INSTALL;
		}
		if ( option == 'x' ) {
			/* configure script options */			
			strcpy ( &CONFIG_OPTS[0], optarg );				
		}
		
		if ( option == 'f' ) {
			FORCE = 1;
		}
		if ( option == 'v' ) {
			VERBOSE = 1;
		}
				
		if ( option == 's' ) {
			SKIP_CFG = 1;
		}
		if ( option == 'o' ) {
			/* GEM_C_OPTS gets passed to the C compiler via the GRASS/GEM Makefiles:
				<EXT>/src/include/Make/Grass.make.in:
					CFLAGS      =  $(INC) $(COMPILE_FLAGS) $(USE_TERMIO) $(GEM_C_OPTS)
			*/			
			strcat (coptions, optarg);
		}
		/* define a custom configure command */
		if ( option == 'C' ) {
			strcpy ( CONFIG_CMD, optarg );
		}
		/* define a custom make command */
		if ( option == 'm' ) {
			strcpy ( MAKE_CMD, optarg );
		}						
		/* get next option from command line */
		option = getopt_long ( argc, argv, ":i:u:q:d:c:t:l:o:x:rhVg:b:fvs", long_options, &option_index );
	}
	
		
	if ( valid < 1 ) {
		print_error ( ERR_INVOCATION,"please specify a valid action.\n" );
	}
	if ( valid > 1 ) {
		print_error ( ERR_INVOCATION,"please specify only one action.\n");
	}
	
	/* export compiler options for use by Makefiles */
	sprintf ( GEM_C_OPTS, "GEM_C_OPTS=%s", coptions );
	putenv ( GEM_C_OPTS );
		
	/* these actions can be done without any extension checking */		
	if ( action == HELP ) {
		/* show usage info and exit */
		show_help ();
		exit (0);	
	}
	
	if ( action == VERSION ) {
		/* show version info and exit */
		show_version ();
		exit (0);
	}

	if (!VERBOSE) {
		/* set temp file to pipe output to for silent operation */
		/* TODO: Do not hardcode temp paths */
		strcpy (TMP_NULL,"/tmp/grass.extension.log.XXXXXX"); /* TMP_NULL is a global variable */
		mkstemp ( TMP_NULL );
		fd = open ( TMP_NULL, O_CREAT );
		if ( fd == -1 ) {
			print_error ( ERR_TMPFILE, "could not create temp file: %s", strerror (errno));
			exit (ERR_TMPFILE);			
		}	
	}
	
	
	/* these actions need a valid GRASS path but no extensions */
	if ( action == RESTORE ) {
		/* figure out path to GRASS installation */
		/* GIS base not given? */
		if ( gisbase == NULL )	 {
			/* try to read from GRASS environment */
			gisbase = getenv ("GISBASE");
			if ( gisbase == NULL) {
				/* still NULL? Abort! */
				print_error ( ERR_INVOCATION, "GISBASE environment variable not set and path to GRASS not given.\n");
			}
		}
		
		if ( VERBOSE ) {
			fprintf (stdout, "Path to GRASS is %s.\n", gisbase);
		}

		restore ( gisbase, grass_version );
		exit (0);
	}
	
	if ( action == LIST ) {		
		/* figure out path to GRASS installation */
		/* GIS base not given? */
		if ( gisbase == NULL )	 {
			/* try to read from GRASS environment */
			gisbase = getenv ("GISBASE");
			if ( gisbase == NULL) {
				/* still NULL? Abort! */
				print_error ( ERR_INVOCATION, "GISBASE environment variable not set and path to GRASS not given.\n");
			}
		}
		
		if ( VERBOSE ) {
			fprintf (stdout, "Path to GRASS is %s.\n", gisbase);
		}
		list_extensions ( gisbase );
		exit (0);		
	}
			
	/* check if extension is stored in a remote URL */
	if ( (strstr (package, "http://")) || (strstr (package, "ftp://")) ) {
		wget_extension ( package ); /* download into current dir using wget */
		/* cut off the path specification */
		url = malloc (sizeof(char) * MAXSTR);
		strcpy (url, package);
		filepart = strrchr ( url, '/' );
		filepart ++;
		strcpy (package, filepart);
		free (url);
	}
	
	if ( VERBOSE ) {
		fprintf (stdout, "Extension location is '%s'.\n", package);
	}
		
	if ( action != UNINSTALL ) {
		error = stat ( package, &buf );
		if ( error < 0 ) {
			print_error ( ERR_NO_ACCESS_EXT, "extension FILE or DIR '%s' invalid: %s\n", package, strerror (errno));
		}
		if ( S_ISDIR ( buf.st_mode ) ) {
			is_directory = 1;
			if ( VERBOSE ) {
				fprintf (stdout, "Extension files stored in a directory.\n");
			}
		} else {
			if ( VERBOSE ) {
				fprintf (stdout, "Extension files stored in a package file.\n");
			}
			/* DECOMPRESS INTO TEMP DIR, CHANGE NAME OF package TO THAT DIR */
			unpack_extension ( package );
			/* find name of directory containing extension files */
			/* very primitive: just picks the first directory */
			dir = opendir (TMPDIR);
			dir_entry = readdir (dir);
			dir_found = 0;
			while ( dir_entry != NULL ) {
				if (
					(strcmp (dir_entry->d_name,".")) &&
					(strcmp (dir_entry->d_name,".."))
				) {
					/* check if it is a directory */
					sprintf ( tmp, "%s/%s", TMPDIR, dir_entry->d_name);	
					stat ( tmp, &buf2 );
					if ( S_ISDIR ( buf2.st_mode ) ) {
						dir_found = 1;				
						break;
					}
				}
				dir_entry = readdir (dir);
			}		
			strcpy ( package, tmp );
		
			if ( dir_found == 0 ) {	
				print_error ( ERR_UNPACK_EXT, "no top-level directory found in extension package.\n");
			}
		}
	}

	/* copy package name into this maliciously named variable */
	/* (sorry about the mess ...) */
	/* This name will be used for all registration actions and */
	/* for creating files and directories that store extension */
	/* information for uninstall and restore actions */
	/* For uninstall, we take the last argument as package, NOT */
	/* file name */
	if ( action == UNINSTALL ) {
		strcpy (pkg_short_name, package);
	} else {		
		get_package_name ( package, pkg_short_name );
	}		

	/* export relevant VARS for use by post script */
	if ( valid > 0 ) {
		/* export all relevant env vars for the post script */
		if ( gisbase == NULL )	 {
			/* try to read from GRASS environment */
			gisbase = getenv ("GISBASE");
		}
		run_post ( package, action, bins, gisbase );
	}
	
	if ( VERBOSE ) {
		fprintf (stdout, "Extension will be installed from '%s'\n", package);
	}
	
	/* CHECK PACKAGE FOR VALIDITY */
	if ( action != UNINSTALL ) {
		check_extension ( package, pkg_name, &pkg_major, &pkg_minor, &pkg_revision );
	}
	
	/* these actions can be done without GRASS checking */	
	if ( action == QUERY ) {
		query_extension ( package, pkg_name, pkg_major, pkg_minor, pkg_revision,
				  pkg_short_name, invocation, orgname );
		exit (0);
	}
	
	if ( action == DETAILS ) {
		show_details ( package );
		exit (0);
	}

	if ( action == LICENSE ) {
		show_license ( package );
		exit (0);
	}
	
	if ( action == CLEAN ) {
		source_clean ( package );
		exit (0);
	}
	
	
	/* The following checks need to be done for all other actions! */
	
	/* figure out path to GRASS installation */
	/* GIS base not given? */
	if ( gisbase == NULL )	 {
		/* try to read from GRASS environment */
		gisbase = getenv ("GISBASE");
		if ( gisbase == NULL) {
			/* still NULL? Abort! */
			print_error ( ERR_INVOCATION, "GISBASE environment variable not set and path to GRASS not given.\n");
		}
	}
	
	if ( VERBOSE ) {
		fprintf (stdout, "Path to GRASS is %s.\n", gisbase);
	}
	
	/* figure out GRASS version */
	grass_version = getenv ("GRASS_VERSION");
	if ( grass_version == NULL ) {
		/* GRASS version can be read from gisbase/etc/VERSIONNUMBER */
		sprintf ( version_file, "%s/etc/VERSIONNUMBER", gisbase );
		f = fopen ( version_file, "r" );
		if ( f == NULL ) {
			/* still NULL? Abort! */
			print_error (ERR_VERSION, "Could not read GRASS version. Did you specify the right path?\n");
		} else {
			grass_version = malloc ( sizeof(char) * 16);
			error = fscanf (f, "%s", grass_version);
			fclose (f);
			if ( error < 1 ) {
				print_error (ERR_VERSION, "Could not read GRASS version. Did you specify the right path?\n");			
			}
		}
	}	
	
	if ( grass_version != NULL ) {
		/* extract major and minor version numbers */	
		tmp = strdup ( grass_version );
	
		grass_major = strtok ( tmp, "." );
		grass_minor = strtok (NULL, "." );
		grass_revision = strtok (NULL, "." );
	
		major = strtol ( grass_major, NULL, 10 );
		minor = strtol ( grass_minor, NULL, 10 );
		revision = strtol ( grass_revision, NULL, 10 );
		
		grass_version = malloc (sizeof (char) * MAXSTR);
		sprintf (grass_version, "%i.%i.%i",  major, minor, revision);
		if ( VERBOSE ) {	
			fprintf (stdout, "GRASS version is %s.\n", grass_version);
		}
		
		if ( major < 6 ) {
			print_error ( ERR_VERSION, "extensions only work with GRASS version 6 and above.\n");
		}
	}

	/* for GDAL compatibility and		
	   for the sake of people using Lorenzo Moretti's GRASS for MacOS, we always export the following
	   compiler options */
		sprintf (coptions, "-L%s/lib -I/usr/local/grasslib/include/ ", gisbase);	
	
	/* these actions can only be done after everything has been checked */
	if ( action == INSTALL ) {
		source_install ( package, gisbase, pkg_short_name, pkg_major, pkg_minor, pkg_revision, grass_version );
		exit (0);
	}	

	if ( action == UNINSTALL ) {
		uninstall ( package, pkg_short_name, gisbase, grass_version );
		exit (0);
	}	

	if ( action == TEST_INSTALL ) {
		test_install ( package, gisbase, pkg_short_name, pkg_major, pkg_minor, pkg_revision, grass_version );
		exit (0);
	}	
	
	if ( action == BIN_INSTALL ) {
		if ( binaries_exist ( package, bins )) {
			bin_install ( package, gisbase, bins, pkg_short_name, pkg_major, pkg_minor, pkg_revision, grass_version );
			exit (0);
		} else {
			print_error ( ERR_MISSING_BINS,"no binaries for system '%s'\n", bins); 
		}		
	}		
	
	exit (0);		
}
