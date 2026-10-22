MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

TARGETS := $(patsubst %.py,$(UTILSDIR)/%.py,gmd.py g.md2man.py)

default: $(TARGETS)

$(UTILSDIR)/g.md2man.py: g.md2man.py
	$(INSTALL) $< $@

$(UTILSDIR)/%.py: %.py
	$(INSTALL_DATA) $< $@
