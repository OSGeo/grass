MODULE_TOPDIR = ../..

PGM=g.version

# cat the COPYING file, add a c line-break \n at each line end
# and remove the unix newline. 

EXTRA_CFLAGS = $(PROJINC) \
	-DGRASS_VERSION_NUMBER=\"'$(GRASS_VERSION_NUMBER)'\" \
	-DGRASS_VERSION_DATE=\"'$(GRASS_VERSION_DATE)'\" \
	-DGRASS_VERSION_SVN=\"'$(GRASS_VERSION_SVN)'\" \
	-DGRASS_VERSION_UPDATE_PKG=\"'$(GRASS_VERSION_UPDATE_PKG)'\"

LIBES = $(GISLIB) $(GEOSLIBS) $(GDALLIBS)
DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

