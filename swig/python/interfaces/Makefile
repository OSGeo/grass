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
#	gis.h  (gis.i based on gis.h c.12/17/04 (r16393)

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
#	generate dig_types.i from include/vect/dig_defines.h
#
#imagery:
#	imagedefs.h
#	imagery.h

distclean:
	-rm -f gisdefs.i dig_structs.i vect.i
	# gis.i dig_types.i imagedefs.h imagery.h
