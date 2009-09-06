
MODULE_TOPDIR = ../..

PGM = d.what.vect

LIBES        = $(DISPLAYLIB) $(VECTLIB) $(FORMLIB) $(DBMILIB) $(GISLIB)

DEPENDENCIES = $(DISPLAYDEP) $(VECTDEP) $(DBMIDEP) $(FORMDEP) $(GISDEP)

EXTRA_CFLAGS = $(VECT_CFLAGS)
EXTRA_INC = $(VECT_INC)

include $(MODULE_TOPDIR)/include/Make/Module.make

# substitute OSX arch flags for tcltk
ifneq ($(MACOSX_ARCHS),)
CFLAGS := $(subst $(MACOSX_ARCHS),,$(CFLAGS)) $(MACOSX_ARCHS_TCLTK)
LDFLAGS := $(subst $(MACOSX_ARCHS),,$(LDFLAGS)) $(MACOSX_ARCHS_TCLTK)
endif

default: cmd




