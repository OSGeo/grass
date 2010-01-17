MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

SRCH := $(wildcard ../include/*.h ../include/vect/*.h ../include/iostream/*.h)
DSTH := $(patsubst ../include/%.h,$(ARCH_INCDIR)/%.h,$(SRCH))

SRCMAKE := $(wildcard ../include/Make/*.make)
DSTMAKE := $(patsubst ../include/Make/%.make,$(ARCH_DISTDIR)/include/Make/%.make,$(SRCMAKE))

default: $(DSTH) $(DSTMAKE)

# cancel rule from Rules.make
$(ARCH_INCDIR)/%.h: %.h

$(ARCH_INCDIR)/vect/%.h: vect/%.h | $(ARCH_INCDIR)/vect
	$(INSTALL_DATA) $< $@

$(ARCH_INCDIR)/iostream/%.h: iostream/%.h | $(ARCH_INCDIR)/iostream
	$(INSTALL_DATA) $< $@

$(ARCH_INCDIR)/%.h: %.h | $(ARCH_INCDIR)
	$(INSTALL_DATA) $< $@

$(ARCH_INCDIR)/vect: | $(ARCH_INCDIR)
	$(MKDIR) $@

$(ARCH_INCDIR)/iostream: | $(ARCH_INCDIR)
	$(MKDIR) $@

$(ARCH_DISTDIR)/include/Make/%.make: Make/%.make | $(ARCH_DISTDIR)/include/Make
	$(INSTALL_DATA) $< $@

$(ARCH_DISTDIR)/include/Make:
	$(MKDIR) $@

