MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

ETCDIR = $(ETC)/gui/images

FILES := $(wildcard *.png)

default: $(patsubst %,$(ETCDIR)/%,$(FILES))

$(ETCDIR)/%.png: %.png | $(ETCDIR)
	$(INSTALL_DATA) $< $@

$(ETCDIR):
	$(MKDIR) $@
