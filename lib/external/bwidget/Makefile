MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make

BWIDGETDIR = $(ARCH_DISTDIR)/bwidget

SRCFILES := $(wildcard images/*.* lang/*.* *.tcl *.txt) README.grass
DSTFILES := $(patsubst %,$(BWIDGETDIR)/%,$(SRCFILES))

ifneq ($(strip $(TCLTKLIBS)),)
default:
	$(MAKE) $(DSTFILES)
endif

$(BWIDGETDIR)/%: % | $(BWIDGETDIR) $(BWIDGETDIR)/images $(BWIDGETDIR)/lang
	$(INSTALL_DATA) $< $@

$(BWIDGETDIR) $(BWIDGETDIR)/images $(BWIDGETDIR)/lang: %:
	$(MKDIR) $@
