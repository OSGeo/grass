#!/bin/sh

set -e

if [ "$CI" ] ; then
	# dumpbin in GH actions does not support options starting with "-"
	DUMPBIN_EXPORT="/EXPORTS"
else
	DUMPBIN_EXPORT="-exports"
fi

[ -d mswindows/osgeo4w/vc ] || mkdir mswindows/osgeo4w/vc

if [ -n "$VCPATH" ]; then
	PATH=$PATH:$VCPATH
fi

for dllfile in "$@"; do
	dlldir=${dllfile%/*}
	dllfile=${dllfile##*/}

	dllbase=${dllfile%.dll}
	dllname=${dllbase#lib}
	dllname=${dllname%.$VERSION}
	defname=$dllname.def
	libname=$dllname.lib

 	echo "$dllfile => $dllname"

	(cd $dlldir; dumpbin "$DUMPBIN_EXPORT" $dllfile) |
		sed -nf mswindows/osgeo4w/mklibs.sed |
		egrep -v "^[	 ]*(_+IMPORT_DESCRIPTOR_.*|_+NULL_IMPORT_DESCRIPTOR)$" >mswindows/osgeo4w/vc/${defname%$VERSION}

	(cd mswindows/osgeo4w/vc ;
	    lib -nologo -def:${defname} -subsystem:windows -machine:x64
	    lib -nologo $libname || exit)
done
