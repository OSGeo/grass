MODULE_TOPDIR = ../..

PGM=g.version

# cat the COPYING file, add a c line-break \n at each line end
# and remove the unix newline. 
COPYING := $(shell cat ./../../COPYING | sed -f sed.script | tr -d '\012')
GRASS_CONFIGURE_PARAMS := $(shell head -n 7 ./../../config.status | tail -n 1 | tr -d '\012')

EXTRA_CFLAGS=-DGRASS_VERSION_NUMBER=\"'$(GRASS_VERSION_NUMBER)'\" -DGRASS_VERSION_DATE=\"'$(GRASS_VERSION_DATE)'\" -DGRASS_VERSION_UPDATE_PKG=\"'$(GRASS_VERSION_UPDATE_PKG)'\"  -DGRASS_CONFIGURE_PARAMS="\"$(GRASS_CONFIGURE_PARAMS)\"" -DCOPYING="\"$(COPYING)\""

LIBES     = $(GISLIB)
DEPENDENCIES= $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd $(GRASS_VERSION_FILE) $(GRASS_BUILD_FILE)

$(GRASS_VERSION_FILE): $(MODULE_TOPDIR)/COPYING
	cat $< | sed -f sed.script | tr -d '\012' > $@

$(GRASS_BUILD_FILE): $(MODULE_TOPDIR)/config.status
	head -n 7 $< | tail -n 1 | sed 's+#++1' | tr -d '\012' > $@
