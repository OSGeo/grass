# MODULE_TOPDIR = ../../..

# LIB = NETA

# LIBES = $(VECTORLIB) $(DBMILIB) $(GISLIB) $(GRAPHLIB)
# DEPENDENCIES= $(VECTORDEP) $(DBMIDEP) $(GISDEP)
# EXTRA_INC = $(VECT_INC)
# EXTRA_CFLAGS = $(VECT_CFLAGS)

# include $(MODULE_TOPDIR)/include/Make/Lib.make
# include $(MODULE_TOPDIR)/include/Make/Doxygen.make

# default: lib

# DOXNAME=neta

build_module(NAME grass_neta
DEPENDS GDAL GEOS
grass_dbmiclient
grass_gis
grass_graph
grass_vector
)

