#!/bin/sh

set -e

PROGRAMFILES="/c/Program Files (x86)"
VSDIR="$PROGRAMFILES/Microsoft Visual Studio 9.0"
PATH="$VSDIR/Common7/IDE:$PATH"
PATH="$VSDIR/VC/bin:$PATH"
PATH="$VSDIR/Common7/Tools:$PATH"
PATH="$PATH:/c/OSGeo4W/bin"
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
		egrep -v "^[ 	]*(_IMPORT_DESCRIPTOR_.*|_NULL_IMPORT_DESCRIPTOR)$" >mswindows/osgeo4w/vc/${defname%$VERSION}

	cd mswindows/osgeo4w/vc
	lib -nologo -def:${defname} -subsystem:windows -machine:x86
	lib -nologo $libname || exit
	cd ../../..
done
