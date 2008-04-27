#!/bin/sh

#% Module
#%  description: GIS manager for GRASS
#% End
#%option
#% key: dmrc
#% type: string
#% description: Name of GIS manager settings file (.grc)
#% required : no
#%End

if [ $# -eq 0 ] ; then
#	if [ "$HOSTTYPE" = "macintosh" -o "$HOSTTYPE" = "powermac" -o "$HOSTTYPE" = "powerpc" -o "$HOSTTYPE" = "intel-pc" ] ; then
#		exec "$GRASS_WISH" "$GISBASE/etc/gm/gm.tcl" -name gm_tcl
#	else
   		exec "$GRASS_WISH" "$GISBASE/etc/gm/gm.tcl" -name gm_tcl &
#   	fi
	exit
fi

if [ "$1" != "@ARGS_PARSED@" ] ; then
  exec g.parser "$0" "$@"
fi

exec "$GRASS_WISH" "$GISBASE/etc/gm/gm.tcl" -name gm_tcl "$GIS_OPT_DMRC" sh &
