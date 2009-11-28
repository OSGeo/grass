MODULE_TOPDIR = ../..

PGM=g.version

# cat the COPYING file, add a c line-break \n at each line end
# and remove the unix newline. 
COPYING := $(shell cat $(MODULE_TOPDIR)/COPYING | sed -f sed.script | tr -d '\012')
GRASS_CONFIGURE_PARAMS := $(shell head -n 7 $(MODULE_TOPDIR)/config.status | tail -n 1 | tr -d '\012')

EXTRA_CFLAGS = \
	-DGRASS_VERSION_NUMBER=\"'$(GRASS_VERSION_NUMBER)'\" \
	-DGRASS_VERSION_DATE=\"'$(GRASS_VERSION_DATE)'\" \
	-DGRASS_VERSION_SVN=\"'$(GRASS_VERSION_SVN)'\" \
	-DGRASS_VERSION_UPDATE_PKG=\"'$(GRASS_VERSION_UPDATE_PKG)'\" \
	-I$(OBJDIR)

EXTRA_HEADERS = $(OBJDIR)/copying.h $(OBJDIR)/confparms.h

LIBES = $(GISLIB)
DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

$(OBJDIR)/copying.h: $(MODULE_TOPDIR)/COPYING | $(OBJDIR)
	sed -e 's/^\(.*\)$$/"\1\\n"/' $< > $@

$(OBJDIR)/confparms.h: $(MODULE_TOPDIR)/config.status | $(OBJDIR)
	sed -n '7s/^#\(.*\)$$/"\1"/p' $< > $@
