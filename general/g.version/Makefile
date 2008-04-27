MODULE_TOPDIR = ../..

PGM=g.version

# cat the COPYING file, add a c line-break \n at each line end
# and remove the unix newline. 
COPYING=`cat ./../../COPYING | sed -f sed.script | tr -d '\012'`
GRASS_CONFIGURE_PARAMS=`head -n 7 ./../../config.status | tail -n 1 | tr -d '\012'`

EXTRA_CFLAGS=-DGRASS_VERSION_NUMBER=\"'$(GRASS_VERSION_NUMBER)'\" -DGRASS_VERSION_DATE=\"'$(GRASS_VERSION_DATE)'\" -DGRASS_VERSION_UPDATE_PKG=\"'$(GRASS_VERSION_UPDATE_PKG)'\"  -DGRASS_CONFIGURE_PARAMS="\"$(GRASS_CONFIGURE_PARAMS)\"" -DCOPYING="\"$(COPYING)\""

LIBES     = $(GISLIB)
DEPENDENCIES= $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd COPYING GRASS_CONFIGURE_PARAMS

COPYING:
	cat ./../../COPYING | sed -f sed.script | tr -d '\012' > $(GRASS_VERSION_FILE)
GRASS_CONFIGURE_PARAMS:
	head -n 7 ./../../config.status | tail -n 1 | sed 's+#++1' | tr -d '\012' > $(GRASS_BUILD_FILE)
