#!/bin/sh
#% Module
#%  description: Display manager for GRASS
#% End
#%option
#% key: dmrc
#% type: string
#% description: Name of .dmrc settings file
#% required : no
#%End

if [ $# -eq 0 ] ; then
	if [ "$HOSTTYPE" = "macintosh" -o "$HOSTTYPE" = "powermac" -o "$HOSTTYPE" = "powerpc" -o "$HOSTTYPE" = "intel-pc" ] ; then
		exec "$GRASS_WISH" $GISBASE/etc/dm/d.m.tcl -name d_m_tcl
	else
   		exec "$GRASS_WISH" $GISBASE/etc/dm/d.m.tcl -name d_m_tcl sh &
   	fi
	exit 0
fi

if [ "$1" != "@ARGS_PARSED@" ] ; then
  exec g.parser "$0" "$@"
fi

exec "$GRASS_WISH" "$GISBASE/etc/dm/d.m.tcl" -name d_m_tcl "$GIS_OPT_DMRC" sh &
