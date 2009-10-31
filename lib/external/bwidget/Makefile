MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make

BWIDGETDIR = $(ARCH_DISTDIR)/bwidget

SRCFILES := $(wildcard images/*.* lang/*.* *.tcl *.txt) README.grass
DSTFILES := $(patsubst %,$(BWIDGETDIR)/%,$(SRCFILES))

default:
	$(MAKE) $(DSTFILES)

$(BWIDGETDIR)/%: % | $(BWIDGETDIR) $(BWIDGETDIR)/images $(BWIDGETDIR)/lang
	$(INSTALL_DATA) $< $@

$(BWIDGETDIR) $(BWIDGETDIR)/images $(BWIDGETDIR)/lang: %:
	$(MKDIR) $@
