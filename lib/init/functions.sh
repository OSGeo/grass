# all exits after setting up $tmp should also tidy it up
cleanup_tmpdir()
{
    # remove session files from tmpdir
    rm -rf "$tmp"
}

help_message()
{
    cat <<-EOF
	Usage:
	  $CMD_NAME [-h | -help | --help] [-v | --version] [-c]
		  [-text | -gui | -tcltk | -wxpython]
		  [[[<GISDBASE>/]<LOCATION_NAME>/]<MAPSET>]
	
	Flags:
	  -h or -help or --help          print this help message
	  -v or --version                show version information and exit
	  -c                             create given mapset if it doesn\'t exist
	  -text                          use text based interface
	                                   and set as default
	  -gui                           use graphical user interface ($DEFAULT_GUI by default)
	                                   and set as default
	  -tcltk                         use Tcl/Tk based graphical user interface
	                                   and set as default
	  -wxpython                      use wxPython based graphical user interface
	                                   and set as default
	
	Parameters:
	  GISDBASE                       initial database (path to GIS data)
	  LOCATION_NAME                  initial location
	  MAPSET                         initial mapset
	
	  GISDBASE/LOCATION_NAME/MAPSET  fully qualified initial mapset directory
	
	Environment variables relevant for startup:
	  GRASS_GUI                      select GUI (text, gui, tcltk, wxpython)
	  GRASS_WISH                     set wish shell name to override 'wish'
	  GRASS_HTML_BROWSER             set html web browser for help pages
	  GRASS_ADDON_PATH               set additional path(s) to local GRASS modules
	  GRASS_BATCH_JOB                shell script to be processed as batch job
	  GRASS_PYTHON                   set python shell name to override 'python'
	EOF
}

create_tmp()
{
## use TMPDIR if it exists, otherwise /tmp
    tmp="${TMPDIR-/tmp}/grass7-$USER-$GIS_LOCK"
    (umask 077 && mkdir "$tmp") || (
	echo "Cannot create temporary directory! Exiting." 1>&2
	exit 1
    )
}

create_gisrc()
{
    # Set the session grassrc file
    GISRC="$tmp/gisrc"
    export GISRC

    # remove invalid GISRC file to avoid disturbing error messages:
    cat "$GISRCRC" 2>/dev/null| grep UNKNOWN >/dev/null
    if [ $? -eq 0 ] ; then
       rm -f "$GISRCRC"
    fi
    
    # Copy the global grassrc file to the session grassrc file
    if [ -f "$GISRCRC" ] ; then
	cp "$GISRCRC" "$GISRC"
	if [ $? -eq 1 ] ; then
	    echo "Cannot copy '$GISRCRC' to '$GISRC'"
	    cleanup_tmpdir
	    exit 1
	fi
    fi
}

read_gui()
{
    # At this point the GRASS user interface variable has been set from the
    # command line, been set from an external environment variable, or is 
    # not set. So we check if it is not set
    if [ ! "$GRASS_GUI" ] ; then
    
	# Check for a reference to the GRASS user interface in the grassrc file
	if [ -f "$GISRC" ] ; then
	    GRASS_GUI=`awk '/GRASS_GUI/ {print $2}' "$GISRC"`
	fi
	
	# Set the GRASS user interface to the default if needed
	if [ ! "$GRASS_GUI" ] ; then
	    GRASS_GUI="$DEFAULT_GUI"
	fi
    fi
    
    if [ "$GRASS_GUI" = "gui" ] ; then
	GRASS_GUI="$DEFAULT_GUI"
    fi

    # d.m no longer exists
    case "$GRASS_GUI" in
    d.m | oldtcltk)
	GRASS_GUI=gis.m
	;;
    esac
}

get_locale()
{
    if [ "$LC_ALL" ] ; then
	LCL=`echo "$LC_ALL" | sed 's/\(..\)\(.*\)/\1/'`
    elif [ "$LC_MESSAGES" ] ; then
	LCL=`echo "$LC_MESSAGES" | sed 's/\(..\)\(.*\)/\1/'`
    else
	LCL=`echo "$LANG" | sed 's/\(..\)\(.*\)/\1/'`
    fi
}

set_paths()
{
    if [ -n "$GRASS_ADDON_PATH" ] ; then
	PATH="$GISBASE/bin:$GISBASE/scripts:$GRASS_ADDON_PATH:$PATH"
    else
	PATH="$GISBASE/bin:$GISBASE/scripts:$PATH"
    fi
    export PATH

    # Set LD_LIBRARY_PATH to find GRASS shared libraries
    if [ ! "GRASS_LD_LIBRARY_PATH" ] ; then
	GRASS_LD_LIBRARY_PATH="$GISBASE/lib"
    else
	GRASS_LD_LIBRARY_PATH="$GISBASE/lib:$GRASS_LD_LIBRARY_PATH"
    fi

# Set PYTHONPATH to find GRASS Python modules
if [ ! "PYTHONPATH" ] ; then
  PYTHONPATH="$GISBASE/etc/python"
else
  PYTHONPATH="$GISBASE/etc/python:$PYTHONPATH"
fi
export PYTHONPATH
}

set_defaults()
{
    # GRASS_PAGER
    if [ ! "$GRASS_PAGER" ] ; then
	if [ -x /bin/more ] || [ -x /usr/bin/more ] ; then
	    GRASS_PAGER=more
	elif [ -x /bin/less ] || [ -x /usr/bin/less ] ; then
	    GRASS_PAGER=less
	elif [ "$MINGW" ] ; then
	    GRASS_PAGER=more
	else
	    GRASS_PAGER=cat
	fi
	export GRASS_PAGER
    fi

    # GRASS_WISH
    if [ ! "$GRASS_WISH" ] ; then
	GRASS_WISH=wish
	export GRASS_WISH
    fi

    # GRASS_PYTHON
    if [ ! "$GRASS_PYTHON" ] ; then
	GRASS_PYTHON=python
	export GRASS_PYTHON
    fi

    # GRASS_GNUPLOT
    if [ ! "$GRASS_GNUPLOT" ] ; then
	GRASS_GNUPLOT="gnuplot -persist"
	export GRASS_GNUPLOT
    fi
}

set_browser()
{
    # GRASS_HTML_BROWSER
    if [ ! "$GRASS_HTML_BROWSER" ] ; then
	if [ "$MACOSX" ] ; then
	    # OSX doesn't execute browsers from the shell PATH - route thru a script
	    GRASS_HTML_BROWSER="$ETC/html_browser_mac.sh"
	    GRASS_HTML_BROWSER_MACOSX="-b com.apple.helpviewer"
	    export GRASS_HTML_BROWSER_MACOSX

	elif [ "$MINGW" -o "$CYGWIN" ] ; then
	    # MinGW startup moved to into init.bat
	    iexplore="$SYSTEMDRIVE/Program Files/Internet Explorer/iexplore.exe"
	    if [ -f "$iexplore" ] ; then
		GRASS_HTML_BROWSER=$iexplore
	    else
		GRASS_HTML_BROWSER="iexplore"
	    fi

	else
	    # the usual suspects
	    BROWSERS="htmlview konqueror mozilla mozilla-firefox firefox opera netscape dillo"
	    for BROWSER in $BROWSERS ; do
		for i in `echo "$PATH" | sed 's/^:/.:/ ; s/::/:.:/g ; s/:$/:./ ; s/:/ /g'` ; do
		    if [ -f "$i/$BROWSER" ] ; then  
			GRASS_HTML_BROWSER="$BROWSER"
			break
		    fi
		done
		if [ -n "$GRASS_HTML_BROWSER" ] ; then
		    break
		fi
	    done
	fi
    fi

    if [ "$MACOSX" -a "$GRASS_HTML_BROWSER" != "" ] ; then
	# OSX doesn't execute browsers from the shell PATH - route thru a script
	GRASS_HTML_BROWSER_MACOSX="-b $GRASS_HTML_BROWSER"
	export GRASS_HTML_BROWSER_MACOSX
	GRASS_HTML_BROWSER="$ETC/html_browser_mac.sh"
    fi

    if [ ! "$GRASS_HTML_BROWSER" ] ; then
	echo "WARNING: Searched for a web browser, but none found." 1>&2
	# even so we set konqueror to make lib/gis/parser.c happy:
	GRASS_HTML_BROWSER=konqueror
    fi
    export GRASS_HTML_BROWSER
}

grass_intro()
{
    if [ ! -f "$GISBASE/locale/$LCL/etc/grass_intro" ] ; then
	cat "$ETC/grass_intro"
    else
	cat "$GISBASE/locale/$LCL/etc/grass_intro"
    fi

    echo
    echo "Hit RETURN to continue"
    read ans

    #for convenience, define pwd as GISDBASE:
    cat > "$GISRC" <<-EOF
	GISDBASE: $PWD
	LOCATION_NAME: <UNKNOWN>
	MAPSET: <UNKNOWN>
	EOF
}

check_gui()
{
    # Check if we are running X windows by checking the DISPLAY variable
    if [ "$DISPLAY" -o "$MINGW" ] ; then

	# Check if python is working properly
	if [ "$GRASS_GUI" = "wxpython" ]; then
	    echo 'variable=True' | "$GRASS_PYTHON" >/dev/null 2>&1
	fi
	# Check if we need to find wish
	if [ "$GRASS_GUI" = "tcltk" ] || \
	    [ "$GRASS_GUI" = "gis.m" ] ; then

	    # Check if wish is working properly
	    echo 'exit 0' | "$GRASS_WISH" >/dev/null 2>&1
	fi

	# ok
	if [ "$?" = 0 ] ; then
	    # Set the tcltkgrass base directory
	    TCLTKGRASSBASE="$ETC"
	    # Set the wxpython base directory
	    WXPYTHONGRASSBASE="$ETC/wxpython"
	else
	    # Wish was not found - switch to text interface mode
	    cat <<-EOF
		
		WARNING: The wish command does not work as expected!
		Please check your GRASS_WISH environment variable.
		Use the -help option for details.
		Switching to text based interface mode.
		
		Hit RETURN to continue.

		EOF
	    read ans

	    GRASS_GUI="text"
	fi
    else
	# Display a message if a graphical interface was expected
	if [ "$GRASS_GUI" != "text" ] ; then
	    # Set the interface mode to text
	    cat <<-EOF
		
		WARNING: It appears that the X Windows system is not active.
		A graphical based user interface is not supported.
		Switching to text based interface mode.
		
		Hit RETURN to continue

		EOF
	    read ans

	    GRASS_GUI="text"
	fi
    fi

    # Save the user interface variable in the grassrc file - choose a temporary
    # file name that should not match another file
    if [ -f "$GISRC" ] ; then
	awk '$1 !~ /GRASS_GUI/ {print}' "$GISRC" > "$GISRC.$$"
	echo "GRASS_GUI: $GRASS_GUI" >> "$GISRC.$$"
	mv -f "$GISRC.$$" "$GISRC"
    fi
}

non_interactive()
{
    # Try non-interactive startup
    L=
    
    if [ "$1" = "-" ] ; then
	
	if [ "$LOCATION" ] ; then
	    L="$LOCATION"
	fi
    else
	L="$1"
    fi

    if [ "$L" ] ; then
	if [ "$L" = "." ] ; then
	    L=$PWD
	elif [ `echo "$L" | cut -c 1` != "/" ] ; then
	    L="$PWD/$L"
	fi

	MAPSET=`basename "$L"`
	L=`dirname "$L"`
	
	if [ "$L" != "." ] ; then
	    LOCATION_NAME=`basename "$L"`
	    L=`dirname "$L"`
	    
	    if [ "$L" != "." ] ; then
		GISDBASE="$L"
	    fi
	fi
    fi

    #strip off white space from LOCATION_NAME and MAPSET: only supported for $GISDBASE
    MAPSET=`echo $MAPSET | sed 's+ ++g'`
    LOCATION_NAME=`echo $LOCATION_NAME | sed 's+ ++g'`

    if [ "$GISDBASE" -a "$LOCATION_NAME" -a "$MAPSET" ] ; then
	LOCATION="$GISDBASE/$LOCATION_NAME/$MAPSET"

	if [ ! -r "$LOCATION/WIND" ] ; then
	    if [ "$LOCATION_NAME" = "PERMANENT" ] ; then
		echo "$LOCATION: Not a valid GRASS location"
		cleanup_tmpdir
		exit 1
	    else
		   # the user wants to create mapset on the fly
		if [ -n "$CREATE_NEW" ] && [ "$CREATE_NEW" -eq 1 ] ; then
		    if [ ! -f "$GISDBASE/$LOCATION_NAME/PERMANENT/DEFAULT_WIND" ] ; then
			echo "The LOCATION \"$LOCATION_NAME\" does not exist. Please create it first"
			cleanup_tmpdir
			exit 1
		    else
			mkdir -p "$LOCATION"
			cp "$GISDBASE/$LOCATION_NAME/PERMANENT/DEFAULT_WIND" "$LOCATION/WIND"
			echo "Missing WIND file fixed"
		    fi
		else
		    echo "$LOCATION: Not a valid GRASS location"
		    cleanup_tmpdir
		    exit 1
		fi
	    fi
	fi

	if [ -s "$GISRC" ] ; then
	    sed -e "s|^GISDBASE:.*$|GISDBASE: $GISDBASE|; \
		s|^LOCATION_NAME:.*$|LOCATION_NAME: $LOCATION_NAME|; \
		s|^MAPSET:.*$|MAPSET: $MAPSET|" "$GISRC" > "$GISRC.$$"
	    
	    if [ $? -eq 0 ] ; then
		mv -f "$GISRC.$$" "$GISRC"
	    else
		rm -f "$GISRC.$$"
		echo "Failed to create new $GISRC"
		LOCATION=
	    fi
	else
	    cat > "$GISRC" <<-EOF
		GISDBASE: $GISDBASE
		LOCATION_NAME: $LOCATION_NAME
		MAPSET: $MAPSET
		EOF
	fi
    else
	echo "GISDBASE, LOCATION_NAME and MAPSET variables not set properly."
	echo "Interactive startup needed."
	cleanup_tmpdir
	exit 1
    fi
}

set_data()
{
    # User selects LOCATION and MAPSET if not set
    if [ ! "$LOCATION" ] ; then
    
	case "$GRASS_GUI" in
    
	    # Check for text interface
	    text)
		;;
	    
	    # Check for GUI
	    tcltk | gis.m | wxpython)
		gui_startup
		;;
	    *)
		# Shouldn't need this but you never know
		echo "ERROR: Invalid user interface specified - <$GRASS_GUI>."
		echo "Use the --help option to see valid interface names."
		cleanup_tmpdir
		exit 1
		;;
	esac
    fi
}

gui_startup()
{
    if [ "$GRASS_GUI" = "tcltk" ] || [ "$GRASS_GUI" = "gis.m" ] ; then
		# eval `foo` will return subshell return code and not app foo return code!!!
	eval '"$GRASS_WISH" -file "$TCLTKGRASSBASE/gis_set.tcl"'
	thetest=$?
    else
	eval '"$GRASS_PYTHON" "$WXPYTHONGRASSBASE/gis_set.py"'
	thetest=$?
    fi

    case $thetest in
	1)
	    # The gis_set.tcl script printed an error message so wait
	    # for user to read it
	    cat <<-EOF
		Error in GUI startup. If necessary, please
		report this error to the GRASS developers.
		Switching to text mode now.

		Hit RETURN to continue...

		EOF
	    read ans

	    exec "$CMD_NAME" -text
	    cleanup_tmpdir
	    exit 1
	    ;;

	0)
	    # These checks should not be necessary with real init stuff
	    if [ "$LOCATION_NAME" = "##NONE##" ] || [ "$LOCATION_NAME" = "##ERROR##" ] ; then
		echo "The selected location is not a valid GRASS location"
		cleanup_tmpdir
		exit 1
	    fi
	    ;;

	2)
	    # User wants to exit from GRASS
	    echo "Received EXIT message from GUI."
	    echo "GRASS is not started. Bye."
	    cleanup_tmpdir
	    exit 0
	    ;;
	*)
	    echo "ERROR: Invalid return code from gis_set.tcl."
	    echo "Please advise GRASS developers of this error."
	    cleanup_tmpdir
	    exit 1
	    ;;
    esac
}

read_gisrc()
{
    GISDBASE=`g.gisenv GISDBASE`
    LOCATION_NAME=`g.gisenv LOCATION_NAME`
    MAPSET=`g.gisenv MAPSET`

    if [ -z "$GISDBASE" ] || [ -z "$LOCATION_NAME" ] || [ -z "$MAPSET" ] ; then
	cat <<-EOF
	ERROR: Reading data path information from g.gisenv.
	GISDBASE=[$GISDBASE]
	LOCATION_NAME=[$LOCATION_NAME]
	MAPSET=[$MAPSET]
	
	Check the <$GISRCRC> file.
	EOF

	cleanup_tmpdir
	exit 1
    fi

    LOCATION="${GISDBASE?}/${LOCATION_NAME?}/${MAPSET?}"
}

check_lock()
{
    # Check for concurrent use
    lockfile="$LOCATION/.gislock"
    "$ETC/lock" "$lockfile" $$
    case $? in
	0) ;;
	1)
	    echo "$USER is currently running GRASS in selected mapset (file $lockfile found). Concurrent use not allowed."
	    cleanup_tmpdir
	    exit 1 ;;
	*)
	    echo Unable to properly access "$lockfile"
	    echo Please notify system personel.
	    cleanup_tmpdir
	    exit 1 ;;
    esac
}

make_fontcap()
{
if [ "$GRASS_FONT_CAP" ] && [ ! -f "$GRASS_FONT_CAP" ] ; then
	echo "Building user fontcap ..."
	g.mkfontcap
fi
}

check_shell()
{
    # cygwin has many problems with the shell setup
    # below, so i hardcoded everything here.
    if [ "$CYGWIN" ] ; then
        sh="cygwin"
        shellname="GNU Bash (Cygwin)"
        export SHELL=/usr/bin/bash.exe
        export OSTYPE=cygwin
    else 
        sh=`basename "$SHELL"`
        case "$sh" in
            ksh)  shellname="Korn Shell";;
            csh)  shellname="C Shell" ;;
            tcsh) shellname="TC Shell" ;;
            bash) shellname="Bash Shell" ;;
            sh)   shellname="Bourne Shell";;
            *)    shellname=shell;;
        esac
    fi

    # check for SHELL
    if [ ! -x "$SHELL" ] ; then
        echo "ERROR: The SHELL variable is not set" 1>&2
        rm -f "$lockfile"
        cleanup_tmpdir
        exit 1
    fi
}

check_batch_job()
{
    # hack to process batch jobs:
    if [ -n "$GRASS_BATCH_JOB" ] ; then
       # defined, but ...
       if [ ! -f "$GRASS_BATCH_JOB" ] ; then
          # wrong file
          echo "Job file '$GRASS_BATCH_JOB' has been defined in"
          echo "the 'GRASS_BATCH_JOB' variable but not found. Exiting."
          echo
          echo "Use 'unset GRASS_BATCH_JOB' to disable batch job processing."
          cleanup_tmpdir
          exit 1
       else
          # right file, but ...
          if [ ! -x "$GRASS_BATCH_JOB" ] ; then
             echo "Please change file permission to 'executable' for '$GRASS_BATCH_JOB'"
             cleanup_tmpdir
             exit 1
          else
             echo "Executing '$GRASS_BATCH_JOB' ..."
             GRASS_GUI="text"
             SHELL="$GRASS_BATCH_JOB"
          fi
       fi
    fi
}

start_gui()
{
    # Start the chosen GUI but ignore text
    if [ "$GRASS_DEBUG" -ne 0 ] ; then
       echo "GRASS GUI should be $GRASS_GUI"
    fi
    
    case "$GRASS_GUI" in
        
        # Check for tcltk interface
        tcltk | gis.m)
            "$GISBASE/scripts/gis.m"
            ;;
        wxpython)
            "$GISBASE/etc/wxpython/scripts/wxgui"
            ;;
    
        # Ignore others
        *)
            ;;
    esac
}

clear_screen()
{
if [ "$MINGW" ] ; then
	:
# TODO: uncomment when PDCurses works.
#	cls
else
	if [ -z "$GRASS_BATCH_JOB" ] && [ "$GRASS_DEBUG" -eq 0 ] ; then
	   tput clear
	fi
fi
}

show_banner()
{
    cat <<EOF
          __________  ___   __________    _______________
         / ____/ __ \/   | / ___/ ___/   / ____/  _/ ___/
        / / __/ /_/ / /| | \__ \\\\_  \\   / / __ / / \\__ \\ 
       / /_/ / _, _/ ___ |___/ /__/ /  / /_/ // / ___/ / 
       \____/_/ |_/_/  |_/____/____/   \____/___//____/  

EOF
}

say_hello()
{
    if [ -f "$GISBASE/locale/$LCL/etc/welcome" ] ; then
	cat "$GISBASE/locale/$LCL/etc/welcome"
    else
	cat "$ETC/welcome"
    fi

}

show_info()
{
    cat <<-EOF
	GRASS homepage:                          http://grass.osgeo.org/
	This version running through:            $shellname ($SHELL)
	Help is available with the command:      g.manual -i
	See the licence terms with:              g.version -c
	EOF
    case "$GRASS_GUI" in
    tcltk | gis.m)
        echo "If required, restart the GUI with:       g.gui tcltk"
        ;;
    wxpython)
        echo "If required, restart the GUI with:       g.gui wxpython"
        ;;
    *)
        echo "Start the GUI with:                      g.gui $DEFAULT_GUI"
        ;;
    esac

    echo "When ready to quit enter:                exit"
    echo
}

csh_startup()
{
    USERHOME="$HOME"      # save original home
    HOME="$LOCATION"
    export HOME
    cshrc="$HOME/.cshrc"
    tcshrc="$HOME/.tcshrc"
    rm -f "$cshrc" "$tcshrc"
    echo "set home = $USERHOME" > "$cshrc"
    echo "set history = 3000 savehist = 3000  noclobber ignoreeof" >> "$cshrc"
    echo "set histfile = $HOME/.history" >> "$cshrc"

    echo "set prompt = '\\" >> "$cshrc"
    echo "Mapset <${MAPSET}> in Location <${LOCATION_NAME}> \\" >> "$cshrc"
    echo "GRASS $GRASS_VERSION > '" >> "$cshrc"
    echo 'set BOGUS=``;unset BOGUS' >> "$cshrc"

    if [ -r "$USERHOME/.grass.cshrc" ]
    then
	cat "$USERHOME/.grass.cshrc" >> "$cshrc"
    fi

    if [ -r "$USERHOME/.cshrc" ]
    then
	grep '^ *set  *mail *= *' "$USERHOME/.cshrc" >> "$cshrc"
    fi

    if [ -r "$USERHOME/.tcshrc" ]
    then
	grep '^ *set  *mail *= *' "$USERHOME/.tcshrc" >> "$cshrc"
    fi

    if [ -r "$USERHOME/.login" ]
    then
	grep '^ *set  *mail *= *' "$USERHOME/.login" >> "$cshrc"
    fi

    echo "set path = ( $PATH ) " | sed 's/:/ /g' >> "$cshrc"

    cp "$cshrc" "$tcshrc"
    "$ETC/run" "$SHELL"
    EXIT_VAL=$?

    HOME="$USERHOME"
    export HOME
}

bash_startup()
{
    # save command history in mapset dir and remember more
    export HISTFILE="$LOCATION/.bash_history"
    if [ -z "$HISTSIZE" ] && [ -z "$HISTFILESIZE" ] ; then 
	export HISTSIZE=3000
    fi

    # instead of changing $HOME, start bash with: --rcfile "$LOCATION/.bashrc" ?
    #   if so, must care be taken to explicity call .grass.bashrc et al for
    #   non-interactive bash batch jobs?
    USERHOME="$HOME"      # save original home
    HOME="$LOCATION"      # save .bashrc in $LOCATION
    export HOME
    bashrc="$HOME/.bashrc"
    rm -f "$bashrc"
    echo "test -r ~/.alias && . ~/.alias" >> "$bashrc"
    echo "PS1='$GRASS GRASS_VERSION ($LOCATION_NAME):\w > '" >> "$bashrc"
    echo "PROMPT_COMMAND=$GISBASE/etc/prompt.sh" >> "$bashrc"

    if [ -r "$USERHOME/.grass.bashrc" ]
    then
        cat "$USERHOME/.grass.bashrc" >> "$bashrc"
    fi

    echo "export PATH=\"$PATH\"" >> "$bashrc"
    echo "export HOME=\"$USERHOME\"" >> "$bashrc" # restore user home path

    "$ETC/run" "$SHELL"
    EXIT_VAL=$?

    HOME="$USERHOME"
    export HOME
}

default_startup()
{
    PS1="GRASS $GRASS_VERSION ($LOCATION_NAME):\w > "
    export PS1

    if [ "$MINGW" ] ; then
	# "$ETC/run" doesn't work at all???
        "$SHELL"
	rm -rf "$LOCATION/.tmp"/*  # remove gis.m session files from .tmp
    else
    	"$ETC/run" "$SHELL"
	EXIT_VAL=$?
    fi
}

done_message()
{
    if [ -x "$GRASS_BATCH_JOB" ] ; then
        echo "Batch job '$GRASS_BATCH_JOB' (defined in GRASS_BATCH_JOB variable) was executed."
        echo "Goodbye from GRASS GIS"
	exit $EXIT_VAL
    else
        echo "Done."
        echo
        echo "Goodbye from GRASS GIS"
        echo
    fi
}

clean_temp()
{
    echo "Cleaning up temporary files ..."
    "$ETC/clean_temp" > /dev/null
}

