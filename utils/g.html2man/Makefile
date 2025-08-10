MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

TARGETS := $(patsubst %.py,$(UTILSDIR)/%.py,ghtml.py ggroff.py g.html2man.py)

default: $(TARGETS)

$(UTILSDIR)/g.html2man.py: g.html2man.py
	$(INSTALL) $< $@

$(UTILSDIR)/%.py: %.py
	$(INSTALL_DATA) $< $@
