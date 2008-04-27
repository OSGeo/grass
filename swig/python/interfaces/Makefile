# auto-generate typedef file

# Note: gis contains the raster data API
all: gis vector

gis:
	echo "/* auto-generate swig typedef file (with some GRASS functions removed) */" > gisdefs.i
	cat ../../../include/gisdefs.h | grep -v '#\|asprintf\|G_snprintf\|G_debug\|G_fatal_error\|G_warning\|G_verbose\|lzw\|G_done_msg\|G_message\|G_important_message' >> gisdefs.i

vector:
	echo "/* auto-generate swig typedef file */" > dig_structs.i
	cat ../../../include/vect/dig_structs.h >> dig_structs.i
	echo "/* auto-generate swig typedef file */" > vect.i
	cat ../../../include/Vect.h | grep -v 'V1_\|V2_' >> vect.i

distclean:
	-rm -f gisdefs.i dig_structs.i vect.i
