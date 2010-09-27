MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

TARGETS := $(patsubst %.py,$(TOOLSDIR)/%.py,html.py groff.py g.html2man.py)

default: $(TARGETS)

$(TOOLSDIR)/%.py: %.py
	$(INSTALL) $< $@
