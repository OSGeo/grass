MODULE_TOPDIR = ../..

PGM = g.mlist

EXTRA_INC = $(REGEXINCPATH)
LIBES = $(GISLIB) $(REGEXLIBPATH) $(REGEXLIB)
DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_REGEX),)
default: cmd
else
default:
endif
