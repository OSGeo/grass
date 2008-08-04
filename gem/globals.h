
/***************************************************************************
 *            globals.h
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


#ifndef _GLOBALS_H
#define _GLOBALS_H

/* put a 
   #define LOCAL
   into main.c ! */

#ifdef LOCAL
#define EXTERN
#else
#define EXTERN extern
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "at_exit_funcs.h"
#include "error.h"
#include "tools.h"
#include "reg_deps.h"
#include "reg_entries.h"
#include "reg_html.h"
#include "actions.h"


#define PROGVERSION 1.03

#define MAXSTR 2048		/* maximum length of strings this program handles */

/* possible actions */
#define NONE 0
#define HELP 1
#define VERSION 2
#define INSTALL 3
#define BIN_INSTALL 4
#define QUERY 5
#define CLEAN 6
#define LICENSE 7
#define TEST_INSTALL 8
#define DETAILS 9
#define UNINSTALL 10
#define RESTORE 11
#define LIST 12

/* error codes */
#define ERR_INVOCATION -1
#define ERR_NO_ACCESS_EXT -2
#define ERR_CONFIGURE_EXT -3
#define ERR_COMPILE_EXT -4
#define ERR_INSTALL_EXT -5
#define ERR_INVALID_EXT -6
#define ERR_UNPACK_EXT -7
#define ERR_RM_TMPDIR -8
#define ERR_MISSING_CMD -9
#define ERR_NO_LICENSE -10
#define ERR_VERSION -11
#define ERR_MISSING_BINS -12
#define ERR_UNINSTALL_EXT -13
#define ERR_SU -14
#define ERR_REGISTER_EXT -15
#define ERR_EXISTS_EXT -16
#define ERR_CHECK_DEPS -17
#define ERR_MISSING_DEPS -18
#define ERR_DEREGISTER_EXT -19
#define ERR_DOWNLOAD -20
#define ERR_REGISTER_ENTRIES_GISMAN -21
#define ERR_DEREGISTER_ENTRIES_GISMAN -22
#define ERR_DUMP_PLAIN_TXT -23
#define ERR_REGISTER_HTML -24
#define ERR_DEREGISTER_HTML -25
#define ERR_RESTORE -26
#define ERR_MISSING_CFG -27
#define ERR_DUMP_HTML -28
#define ERR_LIST -29
#define ERR_TMPFILE -30
#define ERR_RM_TMPFILE -31
#define ERR_REGISTER_ENTRIES_GISMAN2 -32
#define ERR_DEREGISTER_ENTRIES_GISMAN2 -33

#define TYPE_UNKNOWN 0
#define TAR_GZIP 1
#define TAR_BZIP2 2
#define ZIP 3
#define TAR 4

#define TOKEN_SUBMENU 0
#define TOKEN_ENTRY 1
#define TOKEN_COMMAND 2
#define TOKEN_SEPARATOR 3


/* ENVIRONMENT VARIABLES */
EXTERN char GINSTALL_DST[MAXSTR];
EXTERN char GINSTALL_INC[MAXSTR];
EXTERN char GINSTALL_LIB[MAXSTR];
EXTERN char UNINSTALL_BASE[MAXSTR];
EXTERN char GEM_EXT_NAME[MAXSTR];
EXTERN char GEM_EXT_VERSION[MAXSTR];
EXTERN char GEM_EXT_DESCR[MAXSTR];
EXTERN char GEM_EXT_INFO[MAXSTR];
EXTERN char GEM_EXT_DEPS[MAXSTR];
EXTERN char GEM_EXT_BUGS[MAXSTR];
EXTERN char GEM_EXT_AUTHORS[MAXSTR];
EXTERN char GEM_GRASS_DIR[MAXSTR];
EXTERN char GEM_ACTION[MAXSTR];
EXTERN char INSTALL_BASE[MAXSTR];
EXTERN char INSTALL_TYPE[MAXSTR];
EXTERN char GEM_FORCE[MAXSTR];
EXTERN char GEM_VERBOSE[MAXSTR];
EXTERN char GEM_GUI[MAXSTR];
EXTERN char GEM_C_OPTS[MAXSTR];
EXTERN char EXT_BASE[MAXSTR];


/* GLOBAL VARIABLES */
EXTERN int VERBOSE;
EXTERN char TMPDIR[MAXSTR];
EXTERN char TMPDB[MAXSTR];
EXTERN char TMP_GISMAN[MAXSTR];
EXTERN char TMP_DESCR[MAXSTR];
EXTERN char TMP_INFO[MAXSTR];
EXTERN char TMP_DEPS[MAXSTR];
EXTERN char TMP_BUGS[MAXSTR];
EXTERN char TMP_AUTHORS[MAXSTR];
EXTERN char TMP_NULL[MAXSTR];	/* pipe all output that should be hidden to this file */

EXTERN char TMP_HTML[MAXSTR];
EXTERN int TMPCLEAN;
EXTERN int TMPDBCLEAN;
EXTERN int FORCE;
EXTERN int UPGRADE;
EXTERN int SKIP_CFG;

EXTERN char GISMAN_CMD[MAXSTR];
EXTERN char GISMAN2_CMD[MAXSTR];
EXTERN char HTML_CMD[MAXSTR];
EXTERN char QGIS_CMD[MAXSTR];
EXTERN char UNINSTALL_CMD[MAXSTR];
EXTERN char CONFIG_OPTS[MAXSTR];
EXTERN char CONFIG_CMD[MAXSTR];
EXTERN char MAKE_CMD[MAXSTR];

/* stores current working directory */
EXTERN char CWD[MAXSTR];

/* this are used to generate a summary message on exit */
EXTERN int ERROR;		/* error code: set to < 0 on abnormal program exit */
EXTERN int WARNINGS;		/* number of warnings issued during program run */



#endif /* _GLOBALS_H */
