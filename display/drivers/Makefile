
MODULE_TOPDIR = ../..

SUBDIRS = 

include $(MODULE_TOPDIR)/include/Make/Platform.make

# compile PNGdriver if lib available:
ifneq ($(HAVE_SOCKET),)
  SUBDIRS += PNG HTMLMAP PS
ifneq ($(USE_X11),)
  SUBDIRS += XDRIVER
endif
ifneq ($(USE_CAIRO),)
  SUBDIRS += cairo
endif
endif

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: parsubdirs $(ETC)/monitorcap
	$(MKDIR) $(GISBASE)/docs/html
	$(INSTALL_DATA) description.html $(GISBASE)/docs/html/displaydrivers.html

# Touch an empty monitorcap if none exists (e.g. native Windows)
$(ETC)/monitorcap:
	if [ ! -f $@ ] ; then touch $@ ; fi

clean: cleansubdirs
