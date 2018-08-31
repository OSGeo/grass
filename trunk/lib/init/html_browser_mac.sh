#!/bin/sh

# open a help file in the browser specified in GRASS_HTML_BROWSER_MACOSX.  A
# script is used so that it operates like other platforms - all it takes
# is the browser executable command and the file to open.  If it's a web URL,
# open in the user's default browser instead, since Help Viewer will do that
# anyways, yet leave an empty Help Viewer window open.
# 
# William Kyngesburye

# Application (.app) executables can't be run directly from the CLI, or they
# will start as a new process, instead of opening a new window or activating
# the application.  The proper method is to use open with the application
# package as an argument.  This can be done in two ways:
# 
# open -a /path/to/application.app /file/to/open
# 
# or:
# 
# open -b app.signature /file/to/open
# 
# for some known apps it's simpler to use the second option.  Whatever is
# used should be taken care of in init.sh.
# 
# For html files, when using app path method, open still wants to open the
# file in the system default browser, so we're left with signatures-only.

if [ ! "$GRASS_HTML_BROWSER_MACOSX" ] ; then
	# default to Help Viewer
	GRASS_HTML_BROWSER_MACOSX="-b com.apple.helpviewer"
fi

if [ "`echo \"$1\" | grep 'https\?://'`" ] && [ "$GRASS_HTML_BROWSER_MACOSX" = "-b com.apple.helpviewer" ] ; then
	open "$1"
else
	open $GRASS_HTML_BROWSER_MACOSX "$1"
fi
