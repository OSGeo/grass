MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make

BWIDGETDIR = $(ARCH_DISTDIR)/bwidget

SRCFILES := $(wildcard images/*.* lang/*.* *.tcl *.txt) README.grass
DSTFILES := $(patsubst %,$(BWIDGETDIR)/%,$(SRCFILES))

default:
	if [ ! -d $(BWIDGETDIR) ]; then $(MKDIR) $(BWIDGETDIR); fi
	if [ ! -d $(BWIDGETDIR)/images ]; then $(MKDIR) $(BWIDGETDIR)/images; fi
	if [ ! -d $(BWIDGETDIR)/lang ]; then $(MKDIR) $(BWIDGETDIR)/lang; fi
	$(MAKE) $(DSTFILES)

$(BWIDGETDIR)/%: %
	$(INSTALL_DATA) $< $@
