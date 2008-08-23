MODULE_TOPDIR = ..

SUBDIRS =

include $(MODULE_TOPDIR)/include/Make/Vars.make

#compile if OpenGL and Tcl/Tk present:
ifneq ($(USE_OPENGL),)
ifneq ($(strip $(TCLTKLIBS)),)
    SUBDIRS += nviz
endif
endif

#compile if OpenGL present
ifneq ($(USE_OPENGL),)
    SUBDIRS += nviz2
endif

#compile if Motif present:
ifneq ($(strip $(XMLIB)),)
    SUBDIRS += xganim
endif

ifneq ($(USE_X11),)
    SUBDIRS += ximgview
endif

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: parsubdirs


