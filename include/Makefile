MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

SRCMAKE := $(wildcard ../include/Make/*.make)
DSTMAKE := $(patsubst ../include/Make/%.make,$(ARCH_DISTDIR)/include/Make/%.make,$(SRCMAKE))

default: $(DSTMAKE) parsubdirs

$(ARCH_DISTDIR)/include/Make/%.make: Make/%.make | $(ARCH_DISTDIR)/include/Make
	$(INSTALL_DATA) $< $@

$(ARCH_DISTDIR)/include/Make:
	$(MKDIR) $@

SUBDIRS = \
	grass

include $(MODULE_TOPDIR)/include/Make/Dir.make
