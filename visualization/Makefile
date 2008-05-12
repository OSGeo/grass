MODULE_TOPDIR = ..

SUBDIRS =

include $(MODULE_TOPDIR)/include/Make/Platform.make

#compile if OpenGL and Tcl/Tk present:
ifneq ($(strip $(OPENGLLIB)),)
ifneq ($(strip $(TCLTKLIBS)),)
    SUBDIRS += nviz
endif
endif

#compile if Motif present:
ifneq ($(strip $(XMLIB)),)
    SUBDIRS += xganim
endif

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: parsubdirs

clean: cleansubdirs

