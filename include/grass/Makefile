MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

SRCH := $(wildcard ../../include/grass/*.h ../../include/grass/defs/*.h ../../include/grass/vect/*.h ../../include/grass/iostream/*.h)
DSTH := $(patsubst ../../include/grass/%.h,$(ARCH_INCDIR)/%.h,$(SRCH))
DSTH_EXTRA = $(ARCH_INCDIR)/copying.h $(ARCH_INCDIR)/citing.h $(ARCH_INCDIR)/confparms.h

default: $(DSTH) $(DSTH_EXTRA)

# cancel rule from Rules.make
$(ARCH_INCDIR)/%.h: %.h

$(ARCH_INCDIR)/defs/%.h: defs/%.h | $(ARCH_INCDIR)/defs
	$(INSTALL_DATA) $< $@

$(ARCH_INCDIR)/vect/%.h: vect/%.h | $(ARCH_INCDIR)/vect
	$(INSTALL_DATA) $< $@

$(ARCH_INCDIR)/iostream/%.h: iostream/%.h | $(ARCH_INCDIR)/iostream
	$(INSTALL_DATA) $< $@

$(ARCH_INCDIR)/%.h: %.h | $(ARCH_INCDIR)
	$(INSTALL_DATA) $< $@

$(ARCH_INCDIR)/defs: | $(ARCH_INCDIR)
	$(MKDIR) $@

$(ARCH_INCDIR)/vect: | $(ARCH_INCDIR)
	$(MKDIR) $@

$(ARCH_INCDIR)/iostream: | $(ARCH_INCDIR)
	$(MKDIR) $@

$(ARCH_INCDIR)/copying.h: $(MODULE_TOPDIR)/COPYING | $(ARCH_INCDIR)
	sed -e 's/^\(.*\)$$/"\1\\n"/' $< > $@

$(ARCH_INCDIR)/citing.h: $(MODULE_TOPDIR)/CITING | $(ARCH_INCDIR)
	sed -e 's/^\(.*\)$$/"\1\\n"/' $< > $@

$(ARCH_INCDIR)/confparms.h: $(MODULE_TOPDIR)/config.status | $(ARCH_INCDIR)
	$< --config | sed "s/'//g" | sed 's/\(.*\)/".\/configure \1"/' > $@
