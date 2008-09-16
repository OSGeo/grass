# auto-generate typedef file
# variadic functions and #includes are removed.

# Note: gis contains the raster data API

HEADER_LINE1 = "/* auto-generated swig typedef file  `date` */"
HEADER_LINE2 = '/* variadic functions and included headers have been removed */'

all: gis vector

gis:
	echo $(HEADER_LINE1) >  gisdefs.i
	echo $(HEADER_LINE2) >> gisdefs.i
	echo >> gisdefs.i
	cat ../../../include/gisdefs.h | \
		sed -e '/\.\.\..*;/d' -e '/\.\.\./,/;/d' \
		-e '/^#/d' >> gisdefs.i
	echo $(HEADER_LINE1) >  gis.i
	echo >> gis.i
	cat ../../../include/gis.h | \
		sed -e 'N;N;N;N;N;N;N;N;N; s/_Color_Rule_[ \t\n]*{[ \t\n]*\(.*}\) low.*/_Color_Rule_\n{\n    \1 low;\n    \1 high;\n/' \
		| grep -v 'CELL_NODATA' >> gis.i

vector:
	echo $(HEADER_LINE1) >  dig_structs.i
	echo $(HEADER_LINE2) >> dig_structs.i
	echo >> dig_structs.i
	cat ../../../include/vect/dig_structs.h | \
		sed -e '/^#include.*>/d' \
		-e '/^#.*DIG___STRUCTS___/d' >> dig_structs.i
	echo $(HEADER_LINE1) >  vect.i
	echo $(HEADER_LINE2) >> vect.i
	echo >> vect.i
	cat ../../../include/Vect.h | \
		sed -e '/V[12]_.*;/d' -e '/V[12]_/,/;/d' \
		-e '/^#/d' >> vect.i
	echo $(HEADER_LINE1) >  dig_types.i
	echo "/* extracted from include/vect/dig_defines.h */" >> dig_types.i
	echo >> dig_types.i
	cat ../../../include/vect/dig_defines.h | \
		sed -n -e '/GV_POINT/,/GV_LINES/p' >> dig_types.i
#
#imagery:
#	imagedefs.h
#	imagery.h

distclean:
	-rm -f gisdefs.i gis.i dig_structs.i dig_types.i vect.i
	# imagedefs.h imagery.h
