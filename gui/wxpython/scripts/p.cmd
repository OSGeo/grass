#!/bin/sh

#%module
#% description: Wrapper for display commands and pX monitors
#% keywords: display
#%end
#%option
#% key: cmd
#% type: string
#% required: yes
#% multiple: no
#% label: Command to be performed
#% description: Example: "d.rast map=elevation.dem@PERMANENT catlist=1300-1400 -i"
#%end
#%option
#% key: opacity
#% type: string
#% required: no
#% multiple: no
#% key_desc: val
#% description: Opacity level in percentage
#% answer: 100
#%end

if [ -z "$GISBASE" ] ; then
    echo "You must be in GRASS GIS to run this program." 1>&2
    exit 1
fi

if [ "$1" != "@ARGS_PARSED@" ] ; then
    exec g.parser "$0" "$@"
fi

cmdfile="`g.gisenv get=GRASS_PYCMDFILE`"

if [ -e ${cmdfile} ] && [ -n "${cmdfile}" ]; then
    :
else
    g.message -e "GRASS_PYCMDFILE File not found. Run p.mon"
    exit 1
fi

cmd="${GIS_OPT_CMD}"

g.message -d message="$0: ${cmd}"

echo ${cmd} >> ${cmdfile}

exit 0
