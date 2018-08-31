#!/bin/sh

set -e

if [ -z "$OSGEO4W_POSTFIX" ]; then
    OSGEO4W_POSTFIX=""
fi

if [ "$OSGEO4W_POSTFIX" != "64" ]; then
    MACHINE=x86
else
    MACHINE=x64
fi

PROGRAMFILES="/c/Program Files (x86)"
VSDIR="$PROGRAMFILES/Microsoft Visual Studio 14.0"
# add example for MS Visual Studio 2017 Community 64 bit
# VSDIR="$PROGRAMFILES/Microsoft Visual Studio/2017/Community"
PATH="$VSDIR/Common7/IDE:$PATH"
PATH="$VSDIR/VC/bin:$PATH"
# add example for MS Visual Studio 2017 Community 64 bit
# PATH="$VSDIR/VC/Tools/MSVC/14.12.25827/bin/Hostx64/x64:$PATH"
PATH="$VSDIR/Common7/Tools:$PATH"
PATH="$PATH:/c/OSGeo4W${OSGEO4W_POSTFIX}/bin"
export PATH

[ -d mswindows/osgeo4w/vc ] || mkdir mswindows/osgeo4w/vc

for dllfile in "$@"; do
	dlldir=${dllfile%/*}
	dllfile=${dllfile##*/}

	dllbase=${dllfile%.dll}
	dllname=${dllbase#lib}
	dllname=${dllname%.$VERSION}
	defname=$dllname.def
	libname=$dllname.lib

 	echo "$dllfile => $dllname"

	(cd $dlldir; dumpbin -exports $dllfile) |
		sed -nf mswindows/osgeo4w/mklibs.sed |
		egrep -v "^[	 ]*(_+IMPORT_DESCRIPTOR_.*|_+NULL_IMPORT_DESCRIPTOR)$" >mswindows/osgeo4w/vc/${defname%$VERSION}

	(cd mswindows/osgeo4w/vc ;
	    lib -nologo -def:${defname} -subsystem:windows -machine:${MACHINE}
	    lib -nologo $libname || exit)
done
