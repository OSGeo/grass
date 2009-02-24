#!/bin/sh
#############################################################################
#
# MODULE:   	GRASS Initialization
# AUTHOR(S):	Original author unknown - probably CERL
#               Andreas Lange - Germany - andreas.lange@rhein-main.de
#   	    	Huidae Cho - Korea - grass4u@gmail.com
#   	    	Justin Hickey - Thailand - jhickey@hpcc.nectec.or.th
#   	    	Markus Neteler - Germany/Italy - neteler@itc.it
#		Hamish Bowman - New Zealand - hamish_nospam at yahoo,com
# PURPOSE:  	The source file for this shell script is in
#   	    	src/general/init/init.sh. It sets up some environment
#   	    	variables and the lock file. It also parses any remaining
#   	    	command line options for setting the GISDBASE, LOCATION, and/or
#   	    	MAPSET. Finally it starts GRASS with the appropriate user
#   	    	interface and cleans up after it is finished.
# COPYRIGHT:    (C) 2000 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#   	    	License (>=v2). Read the file COPYING that comes with GRASS
#   	    	for details.
#
#############################################################################

trap "echo 'User break!' ; exit" 2 3 15

. "$GISBASE/etc/functions.sh"

# Set default GUI
DEFAULT_GUI="wxpython"

# the following is only meant to be an internal variable for debugging this script.
#  use 'g.gisenv set="DEBUG=[0-5]"' to turn GRASS debug mode on properly.
if [ -z "$GRASS_DEBUG" ] ; then
   GRASS_DEBUG=0
fi

# GRASS_SH is normally just for Windows when not started from a bourne 
# shell. But when starting from Init.sh is still needed for GRASS_GUI (still true for GRASS 7?)
GRASS_SH=/bin/sh
export GRASS_SH

# Set GRASS version number for R interface etc (must be an env_var for MS-Windows)
GRASS_VERSION="@GRASS_VERSION_NUMBER@"
export GRASS_VERSION

# Get the command name
CMD_NAME=@START_UP@

# Get the system name
SYSTEM=`uname -s`
case $SYSTEM in
MINGW*)
	MINGW=1
	;;
CYGWIN*)
	CYGWIN=1
	;;
Darwin*)
	MACOSX=1
	;;
esac

# Set the GIS_LOCK variable to current process id
GIS_LOCK=$$
export GIS_LOCK

# Set the global grassrc file
if [ -n "$GRASS_BATCH_JOB" ] ; then
	GISRCRC="$HOME/.grassrc7.`uname -n`"
	if [ ! -f "$GISRCRC" ] ; then
		GISRCRC="$HOME/.grassrc7"
	fi
else
	GISRCRC="$HOME/.grassrc7"
fi

# Set PATH to GRASS bin, ETC to GRASS etc
ETC="$GISBASE/etc"

# Set the username and working directory
if [ "$MINGW" ] ; then
	PWD=`pwd -W`
	USER="$USERNAME"
	if [ ! "$USER" ] ; then
		USER="user_name"
	fi
else
	PWD=`pwd`
	USER="`whoami`"
fi

# Parse the command-line options
# This can't be put into a function as it modifies argv
for i in "$@" ; do
    case "$i" in
    # Check if the user asked for the version
    -v|--version)
        cat "$GISBASE/etc/license"
        exit
        ;;

    # Check if the user asked for help
    help|-h|-help|--help)
        help_message
        exit
        ;;

    # Check if the -text flag was given
    -text)
        GRASS_GUI="text"
        shift
        ;;

    # Check if the -gui flag was given
    -gui)
        GRASS_GUI="$DEFAULT_GUI"
        shift
        ;;

    # Check if the -wxpython flag was given
    -wxpython | -wx)
        GRASS_GUI="wxpython"
        shift
        ;;

    # Check if the user wants to create a new mapset
    -c)
        CREATE_NEW=1
        shift
        ;;
    esac
done

# Create the temporary directory and session grassrc file
create_tmp

# Create the session grassrc file
create_gisrc

# Ensure GRASS_GUI is set
read_gui

# Get Locale name
get_locale

# Set PATH, @LD_LIBRARY_PATH_VAR@, PYTHONPATH
GRASS_LD_LIBRARY_PATH="$@LD_LIBRARY_PATH_VAR@"
set_paths
@LD_LIBRARY_PATH_VAR@="$GRASS_LD_LIBRARY_PATH"
export @LD_LIBRARY_PATH_VAR@

# Set GRASS_PAGER, GRASS_WISH, GRASS_PYTHON, GRASS_GNUPLOT
set_defaults

# Set GRASS_HTML_BROWSER
set_browser

#predefine monitor size for certain architectures
if [ "$HOSTTYPE" = "arm" ] ; then
   #small monitor on ARM (iPAQ, zaurus... etc)
   GRASS_HEIGHT=320
   GRASS_WIDTH=240
   export GRASS_HEIGHT GRASS_WIDTH
fi

if [ ! "$GRASS_PROJSHARE" ] ; then
    GRASS_PROJSHARE=@CONFIG_PROJSHARE@
    export GRASS_PROJSHARE
fi

# First time user - GISRC is defined in the GRASS script
if [ ! -f "$GISRC" ] ; then
    grass_intro
else
    clean_temp
fi

echo "Starting GRASS ..."

# Check that the GUI works
check_gui "$@"

# Parsing argument to get LOCATION
if [ ! "$1" ] ; then
    # Try interactive startup
    LOCATION=
else
    non_interactive "$@"
fi

# User selects LOCATION and MAPSET if not set
set_data

# Set GISDBASE, LOCATION_NAME, MAPSET, LOCATION from $GISRC
read_gisrc

# Check .gislock file
check_lock

# build user fontcap if specified but not present
make_fontcap

# predefine default driver if DB connection not defined
#  is this really needed?? Modules should call this when/if required.
if [ ! -e "$LOCATION/VAR" ] ; then
   db.connect -c --quiet
fi

trap "" 2 3 15

check_shell

check_batch_job

start_gui

clear_screen
# Display the version and license info
if [ -n "$GRASS_BATCH_JOB" ] ; then
  say_hello
else
  show_banner
  say_hello
  show_info
fi

case "$sh" in
csh|tcsh)
    csh_startup
    ;;

bash|msh|cygwin)
    bash_startup
    ;;

*)
    default_startup
    ;;
esac

trap 2 3 15

clear_screen

clean_temp

rm -f "$lockfile"

# Save GISRC
cp "$GISRC" "$GISRCRC"

cleanup_tmpdir

#### after this point no more grass modules may be called ####

done_message

