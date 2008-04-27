MODULE_TOPDIR = ..

SUBDIRS =

OPENGLBASED = nviz
MOTIFBASED = xganim

include $(MODULE_TOPDIR)/include/Make/Platform.make

#compile if OPENGLBASED present:
ifneq ($(strip $(OPENGLLIB)),)
    SUBDIRS += $(OPENGLBASED)
endif

#compile if MOTIFBASED present:
ifneq ($(strip $(XMLIB)),)
    SUBDIRS += $(MOTIFBASED)
endif

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: parsubdirs

clean: cleansubdirs

